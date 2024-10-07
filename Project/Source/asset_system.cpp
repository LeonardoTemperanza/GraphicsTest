
#include "asset_system.h"
#include "parser.h"

// NOTE: I think the overall design should be this:
// when a user gets an asset in any way, that should be valid
// for the entire duration of the current frame. Only in a frame
// boundary, these can get swapped. If the user needs to hold on to
// an asset for more than one frame, it needs to use an AssetHandle.
// When using an AssetHandle, you also get hot reloading for free.

static Arena sceneArena = ArenaVirtualMemInit(GB(4), MB(2));
static AssetSystem assetManager = {};

Material DefaultMaterial()
{
    Material mat = {};
    mat.pipeline = {};
    mat.uniforms = {};
    mat.textures = {};
    return mat;
}

// Implicit casts
MeshHandle::operator R_Mesh()       { return assetManager.meshes[idx];    }
TextureHandle::operator R_Texture() { return assetManager.textures[idx];  }
ShaderHandle::operator R_Shader()   { return assetManager.shaders[idx];   }
PipelineHandle::operator Pipeline() { return assetManager.pipelines[idx]; }
MaterialHandle::operator Material() { return assetManager.materials[idx]; }

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
#endif

void InitAssetSystem()
{
    ReserveSlotForDefaultAssets();
}

void ReserveSlotForDefaultAssets()
{
    auto& man = assetManager;
    
    assert(man.meshes.len == 0);
    Append(&man.meshes, R_CreateDefaultMesh());
    
    assert(man.textures.len == 0);
    //Append(&man.textures, R_MakeDefaultTexture());
    
    assert(man.shaders.len == 0);
    Append(&man.shaders, R_CreateDefaultShader(ShaderKind_Vertex));
    Append(&man.shaders, R_CreateDefaultShader(ShaderKind_Pixel));
    
    assert(man.pipelines.len == 0);
    R_Shader shaders[] = {man.shaders[0], man.shaders[1]};
    R_Pipeline obj = R_CreatePipeline(ArrToSlice(shaders));
    Pipeline defaultPipeline = {.vert = 0, .pixel = 1, .obj = obj};
    Append(&man.pipelines, defaultPipeline);
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
        *mesh = R_CreateDefaultMesh();
        return;
    }
    
    char** cursor;
    char* c = (char*)contents.ptr;
    cursor = &c;
    
    String magicBytes = Next(cursor, sizeof("mesh")-1);  // Excluding null terminator
    if(magicBytes != "mesh")
    {
        Log("Attempted to load file '%.*s' as a mesh, which it is not.", StrPrintf(path));
        *mesh = R_CreateDefaultMesh();
        return;
    }
    
    u32 version = Next<u32>(cursor);
    if(version != 0)
    {
        Log("Attempted to load file '%.*s' as a mesh, but its version is unsupported.", StrPrintf(path));
        *mesh = R_CreateDefaultMesh();
        return;
    }
    
    char* headerPtr = *cursor;
    auto header = Next<MeshHeader_v0>(cursor);
    
    if(header.isSkinned)
    {
        Log("Skinned meshes are not yet supported.");
        *mesh = R_CreateDefaultMesh();
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
    Append(&assetManager.meshes, {});
    String newStr = ArenaPushString(&sceneArena, path);
    Append(&assetManager.pathToMesh, newStr, {(u32)assetManager.meshes.len - 1});
    LoadMesh(&assetManager.meshes[assetManager.meshes.len - 1], str);
}

MeshHandle GetMeshByPath(const char* path)
{
    auto& man = assetManager;
    
    auto res = Lookup(&man.pathToMesh, ToLenStr(path));
    if(!res.ok)
    {
        MeshHandle mesh;
        Append(&man.meshes, {});
        mesh.idx = man.meshes.len - 1;
        LoadMesh(&man.meshes[mesh.idx], ToLenStr(path));
        String newStr = ArenaPushString(&sceneArena, path);
        Append(&man.pathToMesh, newStr, mesh);
        return mesh;
    }
    
    return *res.res;
}

ShaderHandle GetShaderByPath(const char* path, ShaderKind kind)
{
    auto& man = assetManager;
    
    String str = ToLenStr(path);
    auto res = Lookup(&man.pathToShader, str);
    if(!res.ok)
    {
        ShaderHandle shader;
        Append(&man.shaders, {});
        shader.idx = man.shaders.len - 1;
        LoadShader(&man.shaders[shader.idx], str, kind);
        String newStr = ArenaPushString(&sceneArena, path);
        Append(&man.pathToShader, newStr, shader);
        return shader;
    }
    
    return *res.res;
}

MaterialHandle GetMaterialByPath(const char* path)
{
    auto& man = assetManager;
    
    String str = ToLenStr(path);
    auto res = Lookup(&man.pathToMaterial, str);
    if(!res.ok)
    {
        MaterialHandle material;
        Append(&man.materials, {});
        material.idx = man.materials.len - 1;
        LoadMaterial(&man.materials[material.idx], str);
        String newStr = ArenaPushString(&sceneArena, path);
        Append(&man.pathToMaterial, newStr, material);
        return material;
    }
    
    return *res.res;
}

PipelineHandle GetPipelineByPath(const char* vert, const char* pixel)
{
    ShaderHandle vertHandle  = GetShaderByPath(vert, ShaderKind_Vertex);
    ShaderHandle pixelHandle = GetShaderByPath(pixel, ShaderKind_Pixel);
    return GetPipelineByHandles(vertHandle, pixelHandle);
}

PipelineHandle GetPipelineByHandles(ShaderHandle vert, ShaderHandle pixel)
{
    auto& man = assetManager;
    
    // @speed This lookup could maybe be faster. Probably does not matter
    int found = -1;
    for(int i = 0; i < man.pipelines.len; ++i)
    {
        auto& pipeline = man.pipelines[i];
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
    
    Append(&man.pipelines, pipeline);
    PipelineHandle res = {};
    res.idx = man.pipelines.len - 1;
    return res;
}

AssetMetadata GetMetadata(u32 handle, AssetKind kind)
{
#ifdef BoundsChecking
    assert(kind < Asset_Count && kind >= 0);
#endif
    return assetManager.metadata[kind][handle];
}

void LoadShader(R_Shader* shader, String path, ShaderKind kind)
{
    ScratchArena scratch;
    
    bool success = true;
    String contents = LoadEntireFile(path, scratch, &success);
    if(!success)
    {
        Log("Failed to load file '%.*s'\n", StrPrintf(path));
        *shader = R_CreateDefaultShader(kind);
        return;
    }
    
    char** cursor;
    char* c = (char*)contents.ptr;
    cursor = &c;
    
    String magicBytes = Next(cursor, sizeof("shader")-1);  // Excluding null terminator
    if(magicBytes != "shader")
    {
        Log("%.*s", StrPrintf(magicBytes));
        Log("Attempted to load file '%.*s' as a shader, which it is not.", StrPrintf(path));
        *shader = R_CreateDefaultShader(kind);
        return;
    }
    
    u32 version = Next<u32>(cursor);
    if(version < 0 || version > 1)
    {
        Log("Attempted to load file '%.*s' as a shader, but its version is unsupported.", StrPrintf(path));
        *shader = R_CreateDefaultShader(kind);
        return;
    }
    
    ShaderBinaryHeader_v0 header_v0 = {};
    ShaderBinaryHeader_v1 header_v1 = {};
    
    char* headerPtr = *cursor;
    switch(version)
    {
        case 0: header_v0 = Next<ShaderBinaryHeader_v0>(cursor); Log("v0"); break;
        case 1: header_v1 = Next<ShaderBinaryHeader_v1>(cursor); Log("v1"); break;
        default: assert(false);
    }
    
    // Convert to the final version in an assembly line fashion
    // (first 0 to 1, then 1 to 2, etc)
    switch(version)
    {
        case 0:
        {
            header_v1.v0 = header_v0;
            header_v1.d3d11Bytecode = 0;
            header_v1.d3d11BytecodeSize = 0;
            
        }  // Fallthrough
        case 1:
        {
            // Got to final version.
            // No need to do anything here
            break;
        }
        default: assert(false);
    }
    
    auto& header = header_v1;
    if(header.v0.shaderKind != kind)
    {
        const char* desiredKindStr = GetShaderKindString(kind);
        const char* actualKindStr  = GetShaderKindString((ShaderKind)header.v0.shaderKind);
        Log("Attempted to load shader '%.*s' as a %s, but it's a %s", StrPrintf(path), desiredKindStr, actualKindStr);
        *shader = R_CreateDefaultShader(kind);
        return;
    }
    
    ShaderInput input = {};
    input.dxil          = {.ptr=headerPtr+header.v0.dxil, .len=header.v0.dxilSize};
    input.vulkanSpirv   = {.ptr=headerPtr+header.v0.vulkanSpirv, .len=header.v0.vulkanSpirvSize};
    input.glsl          = {.ptr=headerPtr+header.v0.glsl, .len=header.v0.glslSize};
    input.d3d11Bytecode = {.ptr=headerPtr+header.d3d11Bytecode, .len=header.d3d11BytecodeSize};
    *shader = R_CompileShader((ShaderKind)header.v0.shaderKind, input);
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

void UnloadScene(EntityManager* entMan)
{
    
}

void LoadScene(EntityManager* man, const char* path)
{
    
}
