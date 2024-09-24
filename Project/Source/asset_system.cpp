
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
MeshHandle::operator R_Mesh()       { return assetSystem.meshes[idx];    }
TextureHandle::operator R_Texture() { return assetSystem.textures[idx];  }
ShaderHandle::operator R_Shader()   { return assetSystem.shaders[idx];   }
PipelineHandle::operator Pipeline() { return assetSystem.pipelines[idx]; }
MaterialHandle::operator Material() { return assetSystem.materials[idx]; }

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
    R_Pipeline obj = R_CreatePipeline(ArrToSlice(shaders));
    Pipeline defaultPipeline = {.vert = 0, .pixel = 1, .obj = obj};
    Append(&sys.pipelines, defaultPipeline);
}

void UseMaterial(Material mat)
{
    Pipeline pipeline = mat.pipeline;
    R_SetPipeline(pipeline.obj);
    R_SetUniforms(ToSlice(&mat.uniforms));
    
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
    String newStr = ArenaPushString(&sceneArena, path);
    Append(&assetSystem.pathToMesh, newStr, {(u32)assetSystem.meshes.len - 1});
    LoadMesh(&assetSystem.meshes[assetSystem.meshes.len - 1], str);
}

MeshHandle GetMeshByPath(const char* path)
{
    auto& sys = assetSystem;
    
    auto res = Lookup(&sys.pathToMesh, ToLenStr(path));
    if(!res.ok)
    {
        MeshHandle mesh;
        Append(&sys.meshes, {});
        mesh.idx = sys.meshes.len - 1;
        LoadMesh(&sys.meshes[mesh.idx], ToLenStr(path));
        String newStr = ArenaPushString(&sceneArena, path);
        Append(&sys.pathToMesh, newStr, mesh);
        return mesh;
    }
    
    return res.res;
}

ShaderHandle GetShaderByPath(const char* path, ShaderKind kind)
{
    auto& sys = assetSystem;
    
    String str = ToLenStr(path);
    auto res = Lookup(&sys.pathToShader, str);
    if(!res.ok)
    {
        ShaderHandle shader;
        Append(&sys.shaders, {});
        shader.idx = sys.shaders.len - 1;
        LoadShader(&sys.shaders[shader.idx], str, kind);
        String newStr = ArenaPushString(&sceneArena, path);
        Append(&sys.pathToShader, newStr, shader);
        return shader;
    }
    
    return res.res;
}

MaterialHandle GetMaterialByPath(const char* path)
{
    auto& sys = assetSystem;
    
    String str = ToLenStr(path);
    auto res = Lookup(&sys.pathToMaterial, str);
    if(!res.ok)
    {
        MaterialHandle material;
        Append(&sys.materials, {});
        material.idx = sys.materials.len - 1;
        LoadMaterial(&sys.materials[material.idx], str);
        String newStr = ArenaPushString(&sceneArena, path);
        Append(&sys.pathToMaterial, newStr, material);
        return material;
    }
    
    return res.res;
}

PipelineHandle GetPipelineByPath(const char* vert, const char* pixel)
{
    ShaderHandle vertHandle  = GetShaderByPath(vert, ShaderKind_Vertex);
    ShaderHandle pixelHandle = GetShaderByPath(pixel, ShaderKind_Pixel);
    return GetPipelineByHandles(vertHandle, pixelHandle);
}

PipelineHandle GetPipelineByHandles(ShaderHandle vert, ShaderHandle pixel)
{
    auto& sys = assetSystem;
    
    // @speed This lookup could maybe be faster. Probably does not matter
    int found = -1;
    for(int i = 0; i < sys.pipelines.len; ++i)
    {
        auto& pipeline = sys.pipelines[i];
        if(pipeline.vert.idx == vert.idx && pipeline.pixel.idx == vert.idx)
        {
            found = i;
            break;
        }
    }
    
    if(found != -1) return {(u32)found};
    
    R_Shader shaders[] = {vert, pixel};
    Pipeline pipeline = {};
    pipeline.obj = R_CreatePipeline(ArrToSlice(shaders));
    pipeline.vert  = vert;
    pipeline.pixel = pixel;
    
    Append(&sys.pipelines, pipeline);
    PipelineHandle res = {};
    res.idx = sys.pipelines.len - 1;
    return res;
}

void LoadShader(R_Shader* shader, String path, ShaderKind kind)
{
    ScratchArena scratch;
    
    bool success = true;
    String contents = LoadEntireFile(path, scratch, &success);
    if(!success)
    {
        Log("Failed to load file '%.*s'\n", StrPrintf(path));
        *shader = R_MakeDefaultShader(kind);
        return;
    }
    
    char** cursor;
    char* c = (char*)contents.ptr;
    cursor = &c;
    
    String magicBytes = Next(cursor, sizeof("shader")-1);  // Excluding null terminator
    if(magicBytes != "shader")
    {
        Log("Attempted to load file '%.*s' as a shader, which it is not.", StrPrintf(path));
        *shader = R_MakeDefaultShader(kind);
        return;
    }
    
    u32 version = Next<u32>(cursor);
    if(version != 0)
    {
        Log("Attempted to load file '%.*s' as a shader, but its version is unsupported.", StrPrintf(path));
        *shader = R_MakeDefaultShader(kind);
        return;
    }
    
    char* headerPtr = *cursor;
    ShaderBinaryHeader_v0 header = Next<ShaderBinaryHeader_v0>(cursor);
    if(header.shaderKind != kind)
    {
        const char* desiredKindStr = GetShaderKindString(kind);
        const char* actualKindStr  = GetShaderKindString((ShaderKind)header.shaderKind);
        Log("Attempted to load shader '%.*s' as a %s, but it's a %s", StrPrintf(path), desiredKindStr, actualKindStr);
        *shader = R_MakeDefaultShader(kind);
        return;
    }
    
    String dxil        = {.ptr=headerPtr+header.dxil, .len=header.dxilSize};
    String vulkanSpirv = {.ptr=headerPtr+header.vulkanSpirv, .len=header.vulkanSpirvSize};
    String glsl        = {.ptr=headerPtr+header.glsl, .len=header.glslSize};
    *shader = R_CompileShader((ShaderKind)header.shaderKind, dxil, vulkanSpirv, glsl);
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
    
    const char* vertShaderPath = "";
    const char* pixelShaderPath = "";
    
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
                    vertShaderPath = ArenaPushNullTermString(scratch, strings.b);
                    ConsumeNextLine(&handler);
                }
                else if(strings.a == "pixel_shader")
                {
                    pixelShaderPath = ArenaPushNullTermString(scratch, strings.b);
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
                // TODO
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
    
    // Set pipeline
    {
        ShaderHandle vertHandle = {0};
        ShaderHandle pixelHandle = {1};  // Default pixel handle @cleanup
        
        if(vertShaderPath[0] == '\0')
            Log("Error in material '%.*s': vertex shader not specified.", StrPrintf(path));
        else
            vertHandle = GetShaderByPath(vertShaderPath, ShaderKind_Vertex);
        
        if(pixelShaderPath[0] == '\0')
            Log("Error in material '%.*s': pixel shader not specified.", StrPrintf(path));
        else
            pixelHandle = GetShaderByPath(pixelShaderPath, ShaderKind_Pixel);
        
        material->pipeline = GetPipelineByHandles(vertHandle, pixelHandle);
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
