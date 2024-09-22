
#include "asset_system.h"
#include "parser.h"

static Arena sceneArena = ArenaVirtualMemInit(GB(4), MB(2));

static AssetSystem assetSystem;

Material DefaultMaterial()
{
    Material mat = {};
    mat.vertShaderPath = "";
    mat.pixelShaderPath = "";
    mat.uniforms = {};
    mat.textures = {};
    return mat;
}

AssetKey MakeAssetKey(const char* path)
{
    size_t pathLen = strlen(path);
    
    AssetKey key = {0};
    key.path = {0};
    
    for(int i = 0; i < pathLen; ++i)
        Append(&key.path, path[i]);
    
    Append(&key.path, '\0');
    return key;
}

AssetKey MakeNullAssetKey()
{
    AssetKey key = {};
    return key;
}

void FreeAssetKey(AssetKey* key)
{
    Free(&key->path);
}

R_Mesh LoadMesh(String path, bool* outSuccess)
{
    ScratchArena scratch;
    
    bool success = true;
    String contents = LoadEntireFile(path, scratch, &success);
    if(!success)
    {
        Log("Failed to load file '%.*s'", StrPrintf(path));
        *outSuccess = false;
        return R_MakeDefaultMesh();
    }
    
    char** cursor;
    char* c = (char*)contents.ptr;
    cursor = &c;
    
    String magicBytes = Next(cursor, sizeof("mesh")-1);  // Excluding null terminator
    if(magicBytes != "mesh")
    {
        Log("Attempted to load file '%.*s' as a mesh, which it is not.", StrPrintf(path));
        *outSuccess = false;
        return R_MakeDefaultMesh();
    }
    
    u32 version = Next<u32>(cursor);
    if(version != 0)
    {
        Log("Attempted to load file '%.*s' as a mesh, but its version is unsupported.", StrPrintf(path));
        return R_MakeDefaultMesh();
    }
    
    char* headerPtr = *cursor;
    auto header = Next<MeshHeader_v0>(cursor);
    
    if(header.isSkinned)
    {
        Log("Skinned meshes are not yet supported.");
        *outSuccess = false;
        return R_MakeDefaultMesh();
    }
    
    Slice<Vertex> verts   = {(Vertex*)(headerPtr + header.vertsOffset),   header.numVerts};
    Slice<s32>    indices = {(s32*)   (headerPtr + header.indicesOffset), header.numIndices};
    return R_UploadMesh(verts, indices);
}

R_Texture LoadTexture(String path, bool* outSuccess)
{
    ScratchArena scratch;
    
    bool success = true;
    String contents = LoadEntireFile(path, scratch, &success);
    if(!success)
    {
        Log("Failed to load texture '%.*s'", StrPrintf(path));
        *outSuccess = false;
        
        // TODO: default texture
        TODO;
    }
    
    String stbImage = {0};
    int width, height, numChannels;
    stbImage.ptr = (char*)stbi_load_from_memory((const stbi_uc*)contents.ptr, (int)contents.len, &width, &height, &numChannels, 0);
    stbImage.len = width * height * numChannels;
    if(!stbImage.ptr)
    {
        Log("Failed to load texture '%s'", path);
        u8 fallback[] = {255, 0, 255};
        String s = {.ptr=(const char*)fallback, .len=sizeof(fallback)};
        R_Texture res = R_UploadTexture(s, 1, 1, 3);
        *outSuccess = false;
        return res;
    }
    
    R_Texture res = R_UploadTexture(stbImage, width, height, numChannels);
    stbi_image_free((void*)stbImage.ptr);
    return res;
}

R_Shader LoadShader(String path, ShaderKind kind, bool* outSuccess)
{
    ScratchArena scratch;
    
    bool success = true;
    String contents = LoadEntireFile(path, scratch, &success);
    if(!success)
    {
        Log("Failed to load file '%s'\n", path);
        *outSuccess = false;
        return R_MakeDefaultShader(kind);
    }
    
    char** cursor;
    char* c = (char*)contents.ptr;
    cursor = &c;
    
    String magicBytes = Next(cursor, sizeof("shader")-1);  // Excluding null terminator
    if(magicBytes != "shader")
    {
        Log("Attempted to load file '%s' as a shader, which it is not.", path);
        *outSuccess = false;
        return R_MakeDefaultShader(kind);
    }
    
    u32 version = Next<u32>(cursor);
    if(version != 0)
    {
        Log("Attempted to load file '%s' as a shader, but its version is unsupported.", path);
        *outSuccess = false;
        return R_MakeDefaultShader(kind);
    }
    
    char* headerPtr = *cursor;
    ShaderBinaryHeader_v0 header = Next<ShaderBinaryHeader_v0>(cursor);
    if(header.shaderKind != kind)
    {
        Log("Attempted to load wrong type of shader '%s'", path);
        *outSuccess = false;
        return R_MakeDefaultShader(kind);
    }
    
    String dxil        = {.ptr=headerPtr+header.dxil, .len=header.dxilSize};
    String vulkanSpirv = {.ptr=headerPtr+header.vulkanSpirv, .len=header.vulkanSpirvSize};
    String glsl        = {.ptr=headerPtr+header.glsl, .len=header.glslSize};
    return R_CompileShader((ShaderKind)header.shaderKind, dxil, vulkanSpirv, glsl);
}

Material LoadMaterial(String path, bool* outSuccess)
{
    if(path.len <= 0) return DefaultMaterial();
    
    ScratchArena scratch;
    
    Material mat = DefaultMaterial();
    
    TextFileHandler handler = LoadTextFile(path, scratch);
    if(!handler.ok)
    {
        Log("Could not load material '%.*s'", StrPrintf(path));
        *outSuccess = false;
        return DefaultMaterial();
    }
    
    while(true)
    {
        auto line = ConsumeNextLine(&handler);
        if(!line.ok) break;
        
        if(line.text == ":/material")
        {
            while(true)
            {
                auto matLine = GetNextLine(&handler);
                if(!matLine.ok) break;
                
                auto strings = BreakByChar(matLine, ':');
                if(strings.a == "vertex_shader")
                {
                    // @leak
                    const char* path = ArenaPushNullTermString(&permArena, strings.b);
                    mat.vertShaderPath = path;
                    ConsumeNextLine(&handler);
                }
                else if(strings.a == "pixel_shader")
                {
                    // @leak
                    const char* path = ArenaPushNullTermString(&permArena, strings.b);
                    mat.pixelShaderPath = path;
                    ConsumeNextLine(&handler);
                }
                else
                {
                    break;
                }
            }
        }
        else if(line.text == ":/textures")
        {
            while(true)
            {
                auto texLine = GetNextLine(&handler);
                if(!texLine.ok) break;
                if(StringBeginsWith(texLine.text, ":/")) break;
                
                ConsumeNextLine(&handler);
                // @leak
                String path = ArenaPushString(&permArena, texLine.text);
                Append(&mat.textures, path);
            }
        }
        else if(line.text == ":/uniforms")
        {
            // TODO
        }
        else
        {
            Log("Error in material '%.*s' (line %d): expecting ':/material', ':/textures', or ':/uniforms'", StrPrintf(path), line.num);
            break;
        }
    }
    
    return mat;
}

void UnloadScene()
{
    ArenaFreeAll(&sceneArena);
}

R_Mesh GetMeshByPath(const char* path)
{
    return GetMeshByPath(ToLenStr(path));
}

R_Mesh GetMeshByPath(String path)
{
    if(path.len == 0) return R_MakeDefaultMesh();
    
    int foundIdx = -1;
    for(int i = 0; i < assetSystem.meshPaths.len; ++i)
    {
        if(assetSystem.meshPaths[i] == path)
        {
            foundIdx = i;
            break;
        }
    }
    
    if(foundIdx != -1) return assetSystem.meshes[foundIdx];
    
    bool ok = true;
    R_Mesh res = LoadMesh(path, &ok);
    if(!ok) path = {};
    
    Append(&assetSystem.meshPaths, path);
    Append(&assetSystem.meshes, res);
    return res;
}

R_Texture GetTextureByPath(const char* path)
{
    return GetTextureByPath(ToLenStr(path));
}

R_Texture GetTextureByPath(String path)
{
    // TODO Default texture
    if(path.len == 0) TODO;
    
    int foundIdx = -1;
    for(int i = 0; i < assetSystem.texturePaths.len; ++i)
    {
        if(assetSystem.texturePaths[i] == path)
        {
            foundIdx = i;
            break;
        }
    }
    
    if(foundIdx != -1) return assetSystem.textures[foundIdx];
    
    bool ok = true;
    R_Texture res = LoadTexture(path, &ok);
    if(!ok) path = {};
    
    Append(&assetSystem.texturePaths, path);
    Append(&assetSystem.textures, res);
    return res;
}

R_Shader GetShaderByPath(const char* path, ShaderKind kind)
{
    return GetShaderByPath(ToLenStr(path), kind);
}

R_Shader GetShaderByPath(String path, ShaderKind kind)
{
    if(path.len == 0) return R_MakeDefaultShader(kind);
    
    int foundIdx = -1;
    for(int i = 0; i < assetSystem.shaderPaths.len; ++i)
    {
        if(assetSystem.shaderPaths[i] == path)
        {
            foundIdx = i;
            break;
        }
    }
    
    if(foundIdx != -1) return assetSystem.shaders[foundIdx];
    
    bool ok = true;
    R_Shader res = LoadShader(path, kind, &ok);
    if(!ok) path = {};
    
    Append(&assetSystem.shaderPaths, path);
    Append(&assetSystem.shaders, res);
    return res;
}

R_Cubemap GetCubemapByPath(const char* path)
{
    return GetCubemapByPath(ToLenStr(path));
}

R_Cubemap GetCubemapByPath(String path)
{
    ScratchArena scratch;
    
    const char* paths[6] = {0};
    
    // NOTE: These need to match the call at the bottom
    const char* addendums[6] =
    {
        "_top", "_bottom", "_left", "_right", "_front", "_back"
    };
    
    String ext = GetPathExtension(path);
    
    for(int i = 0; i < 6; ++i)
    {
        StringBuilder builder = {0};
        UseArena(&builder, scratch);
        String pathNoExt = GetPathNoExtension(path);
        Append(&builder, pathNoExt);
        Append(&builder, addendums[i]);
        Append(&builder, '.');
        Append(&builder, ext);
        NullTerminate(&builder);
        
        paths[i] = ToString(&builder).ptr;
    }
    
    int foundIdx = -1;
    for(int i = 0; i < assetSystem.cubemapPaths.len; ++i)
    {
        if(assetSystem.cubemapPaths[i] == path)
        {
            foundIdx = i;
            break;
        }
    }
    
    if(foundIdx != -1) return assetSystem.cubemaps[foundIdx];
    
    // TODO: This stuff should be in a load cubemap procedure
    String textures[6] = {0};    // If length is zero, it means that the texture was not found
    bool isPlaceholder[6] = {0}; // If given texture was loaded with stb or if it comes from an arena
    int width = 0, height = 0, numChannels = 0;
    
    for(int i = 0; i < 6; ++i)
    {
        int prevWidth = width;
        int prevHeight = height;
        int prevNumChannels = numChannels;
        textures[i].ptr = (char*)stbi_load(paths[i], &width, &height, &numChannels, 0);
        if(!textures[i].ptr)
        {
            Log("Could not load texture '%s' for cubemap", paths[i]);
            isPlaceholder[i] = true;
        }
        else
        {
            bool isDifferentFormat = prevWidth != width && prevHeight != height && prevNumChannels != numChannels;
            if(prevWidth != 0 && isDifferentFormat)
            {
                Log("Textures in cubemap '%s' have different formats", paths[i]);
                isPlaceholder[i] = true;
            }
        }
    }
    
    // At this point, if no texture was found, just use default values
    if(width == 0)
    {
        width = height = 1;
        numChannels = 3;
    }
    
    // Prepare all placeholders
    for(int i = 0; i < 6; ++i)
    {
        if(isPlaceholder[i])
        {
            s64 size = width * height * numChannels;
            u8* ptr = (u8*)ArenaAlloc(scratch, size, 1);
            for(int i = 0; i < size; i += 3)
            {
                ptr[i+0] = 255;
                ptr[i+1] = 0;
                ptr[i+2] = 255;
            }
            
            textures[i].ptr = (const char*)ptr;
            textures[i].len = width * height * numChannels;
        }
    }
    
    // NOTE: This needs to match the array at the top
    R_Cubemap res = R_UploadCubemap(textures[0], textures[1], textures[2],
                                    textures[3], textures[4], textures[5],
                                    width, height, numChannels);;
    Append(&assetSystem.cubemapPaths, path);
    Append(&assetSystem.cubemaps, res);
    
    for(int i = 0; i < 6; ++i)
    {
        // NOTE: Only free the ones that were allocated using stbi and not using the arena
        if(!isPlaceholder[i])
            stbi_image_free((void*)textures[i].ptr);
    }
    
    return res;
}

R_Pipeline GetPipelineByPath(const char* vert, const char* pixel)
{
    return GetPipelineByPath(ToLenStr(vert), ToLenStr(pixel));
}

R_Pipeline GetPipelineByPath(String vertShaderPath, String pixelShaderPath)
{
    int foundIdx = -1;
    for(int i = 0; i < assetSystem.pipelinePaths.len; ++i)
    {
        if(assetSystem.pipelinePaths[i].vertPath == vertShaderPath &&
           assetSystem.pipelinePaths[i].pixelPath == pixelShaderPath)
        {
            foundIdx = i;
            break;
        }
    }
    
    if(foundIdx != -1) return assetSystem.pipelines[foundIdx];
    
    R_Shader shaders[] =
    {
        GetShaderByPath(vertShaderPath,  ShaderKind_Vertex),
        GetShaderByPath(pixelShaderPath, ShaderKind_Pixel)
    };
    
    R_Pipeline res = R_CreatePipeline(ArrToSlice(shaders));
    Append(&assetSystem.pipelinePaths, {vertShaderPath, pixelShaderPath});
    Append(&assetSystem.pipelines, res);
    return res;
}

Material GetMaterialByPath(const char* path)
{
    return GetMaterialByPath(ToLenStr(path));
}

Material GetMaterialByPath(String path)
{
    int foundIdx = -1;
    for(int i = 0; i < assetSystem.materialPaths.len; ++i)
    {
        if(assetSystem.materialPaths[i] == path)
        {
            foundIdx = i;
            break;
        }
    }
    
    if(foundIdx != -1) return assetSystem.materials[foundIdx];
    
    bool ok = true;
    Material res = LoadMaterial(path, &ok);
    if(!ok) path = {};
    
    Append(&assetSystem.materialPaths, path);
    Append(&assetSystem.materials, res);
    return res;
}

R_Mesh GetMesh(AssetKey key)
{
    String path = {key.path.ptr, key.path.len};
    return GetMeshByPath(path);
}

R_Pipeline GetPipeline(AssetKey vert, AssetKey pixel)
{
    String vertPath = {vert.path.ptr, vert.path.len};
    String pixelPath = {pixel.path.ptr, pixel.path.len};
    return GetPipelineByPath(vertPath, pixelPath);
}

R_Texture GetTexture(AssetKey key)
{
    String path = {key.path.ptr, key.path.len};
    return GetTextureByPath(path);
}

Material GetMaterial(AssetKey key)
{
    String path = {key.path.ptr, key.path.len};
    return GetMaterialByPath(path);
}

bool CheckMaterial(Material mat, R_Pipeline pipeline)
{
    // TODO: Check if the number of textures in the material slot is correct, etc.
    return true;
}

void UseMaterial(AssetKey material)
{
    Material mat = GetMaterial(material);
    R_Pipeline pipeline = GetPipelineByPath(mat.vertShaderPath, mat.pixelShaderPath);
    R_SetPipeline(pipeline);
    R_SetUniforms(ToSlice(&mat.uniforms));
    
    for(int i = 0; i < mat.textures.len; ++i)
    {
        R_Texture texture = GetTextureByPath(mat.textures[i]);
        R_SetTexture(texture, i);
    }
}
