
#include "asset_system.h"
#include "parser.h"

static Arena sceneArena = ArenaVirtualMemInit(GB(4), MB(2));
static AssetSystem assetSystem = {};

Material DefaultMaterial()
{
    Material mat = {};
    mat.pipeline = {};
    mat.uniforms = {};
    mat.textures = {};
    return mat;
}

// Implicit casts
MeshHandle::operator R_Mesh()         { return assetSystem.meshes[idx];    }
TextureHandle::operator R_Texture()   { return assetSystem.textures[idx];  }
ShaderHandle::operator R_Shader()     { return assetSystem.shaders[idx];   }
PipelineHandle::operator R_Pipeline() { return assetSystem.pipelines[idx]; }
MaterialHandle::operator Material()   { return assetSystem.materials[idx]; }

#if 0
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
#endif

void InitAssetSystem()
{
    ReserveSlotForDefaultAssets();
}

void ReserveSlotForDefaultAssets()
{
    auto& sys = assetSystem;
    
    assert(sys.meshes.len == 0);
    Append(&sys.meshes, R_MakeDefaultMesh());
    
    assert(sys.textures.len == 0);
    //Append(&sys.textures, R_MakeDefaultTexture());
    
    assert(sys.shaders.len == 0);
    Append(&sys.shaders, R_MakeDefaultShader(ShaderKind_Vertex));
    Append(&sys.shaders, R_MakeDefaultShader(ShaderKind_Pixel));
    
    assert(sys.pipelines.len == 0);
    R_Shader shaders[] = {sys.shaders[0], sys.shaders[1]};
    R_Pipeline defaultPipeline = R_CreatePipeline(ArrToSlice(shaders));
    Append(&sys.pipelines, defaultPipeline);
}

void UseMaterial(Material mat)
{
    R_SetPipeline(mat.pipeline);
    R_SetUniforms(ToSlice(&mat.uniforms));
    
    // TODO: Set textures and samplers
    for(int i = 0; i < mat.textures.len; ++i)
        R_SetTexture(mat.textures[i], i);
}

void HotReloadAssets()
{
    // TODO: Loop through all filesystem notifications
}

void LoadMesh(R_Mesh* mesh, String path)
{
    ScratchArena scratch;
    
    bool success = true;
    String contents = LoadEntireFile(path, scratch, &success);
    if(!success)
    {
        Log("Failed to load file '%.*s'", StrPrintf(path));
        *mesh = R_MakeDefaultMesh();
        return;
    }
    
    char** cursor;
    char* c = (char*)contents.ptr;
    cursor = &c;
    
    String magicBytes = Next(cursor, sizeof("mesh")-1);  // Excluding null terminator
    if(magicBytes != "mesh")
    {
        Log("Attempted to load file '%.*s' as a mesh, which it is not.", StrPrintf(path));
        *mesh = R_MakeDefaultMesh();
        return;
    }
    
    u32 version = Next<u32>(cursor);
    if(version != 0)
    {
        Log("Attempted to load file '%.*s' as a mesh, but its version is unsupported.", StrPrintf(path));
        *mesh = R_MakeDefaultMesh();
        return;
    }
    
    char* headerPtr = *cursor;
    auto header = Next<MeshHeader_v0>(cursor);
    
    if(header.isSkinned)
    {
        Log("Skinned meshes are not yet supported.");
        *mesh = R_MakeDefaultMesh();
        return;
    }
    
    Slice<Vertex> verts   = {(Vertex*)(headerPtr + header.vertsOffset),   header.numVerts};
    Slice<s32>    indices = {(s32*)   (headerPtr + header.indicesOffset), header.numIndices};
    *mesh = R_UploadMesh(verts, indices);
}

void LoadCubemap(R_Texture* cubemap, String path)
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
    *cubemap = R_UploadCubemap(textures[0], textures[1], textures[2],
                               textures[3], textures[4], textures[5],
                               width, height, numChannels);
}

void LoadMesh(const char* path)
{
    String str = ToLenStr(path);
    Append(&assetSystem.meshes, {});
    Append(&assetSystem.pathToMesh, str, {(u32)assetSystem.meshes.len - 1});
    LoadMesh(&assetSystem.meshes[assetSystem.meshes.len - 1], str);
}

MeshHandle GetMeshByPath(const char* path)
{
    MeshHandle mesh;
    bool found = Lookup(&assetSystem.pathToMesh, ToLenStr(path), &mesh);
    if(!found)
    {
        Append(&assetSystem.meshes, {});
        mesh.idx = assetSystem.meshes.len - 1;
        LoadMesh(&assetSystem.meshes[mesh.idx], ToLenStr(path));
    }
    
    return mesh;
}

MaterialHandle GetMaterialByPath(const char* path)
{
    MaterialHandle material;
    bool found = Lookup(&assetSystem.pathToMaterial, ToLenStr(path), &material);
    if(!found)
    {
        Append(&assetSystem.materials, {});
        material.idx = assetSystem.materials.len - 1;
        LoadMaterial(&assetSystem.materials[material.idx], ToLenStr(path));
    }
    
    return material;
}

void LoadMaterial(Material* material, String path)
{
    if(path.len <= 0)
    {
        *material = DefaultMaterial();
        return;
    }
    
    ScratchArena scratch;
    
    TextFileHandler handler = LoadTextFile(path, scratch);
    if(!handler.ok)
    {
        Log("Could not load material '%.*s'", StrPrintf(path));
        *material = DefaultMaterial();
        return;
    }
    
    *material = DefaultMaterial();
    
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
                    const char* path = ArenaPushNullTermString(&sceneArena, strings.b);
                    //material->vertShaderPath = path;
                    ConsumeNextLine(&handler);
                }
                else if(strings.a == "pixel_shader")
                {
                    const char* path = ArenaPushNullTermString(&sceneArena, strings.b);
                    //material->pixelShaderPath = path;
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
                String path = ArenaPushString(&sceneArena, texLine.text);
                //Append(&material->textures, path);
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
}

void SerializeScene(EntityManager* man, const char* path)
{
    ScratchArena scratch;
    
    // We first want to serialize the currently used assets for our scene
    
    auto bases = man->bases;
    for(int i = 0; i < bases.len; ++i)
    {
        
    }
}

void UnloadScene(EntityManager* man)
{
    
}

void LoadScene(EntityManager* man, const char* path)
{
    
}