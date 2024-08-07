
#include "asset_system.h"
#include "parser.h"

static Arena sceneArena = ArenaVirtualMemInit(GB(4), MB(2));

Model* LoadModelAssetByName(const char* name)
{
    // This will do the name->path mapping
    TODO;
    return nullptr;
}

void MaybeReloadModelAsset(Model* model)
{
    
}

void LoadScene(const char* path)
{
    
}

Model* LoadModel(const char* path)
{
    ScratchArena scratch;
    
    auto res = ArenaZAllocTyped(Model, &sceneArena);
    
    bool success = true;
    String contents = LoadEntireFile(path, &success);
    if(!success)
    {
        Log("Failed to log file '%s'", path);
        return res;
    }
    
    defer { free((void*)contents.ptr); };
    
    char** cursor;
    char* c = (char*)contents.ptr;
    cursor = &c;
    
    String magicBytes = Next(cursor, sizeof("model")-1);  // Excluding null terminator
    // TODO: Error reporting
    if(magicBytes != "model")
    {
        Log("Attempted to load file '%s' as a model, which it is not.", path);
        return res;
    }
    
    s32 version        = Next<s32>(cursor);
    s32 numMeshes      = Next<s32>(cursor);
    s32 numMaterials   = Next<s32>(cursor);
    res->meshes.ptr    = ArenaZAllocArray(Mesh, numMeshes, &sceneArena);
    res->meshes.len    = numMeshes;
    
    char buffer[1024];
    snprintf(buffer, 1024, "Version: %d, Num meshes: %d, Num materials: %d\n", version, numMeshes, numMaterials);
    DebugMessage(buffer);
    
    for(int i = 0; i < numMeshes; ++i)
    {
        auto& mesh = res->meshes[i];
        
        s32 numVerts    = Next<s32>(cursor);
        s32 numIndices  = Next<s32>(cursor);
        s32 materialIdx = Next<s32>(cursor);
        bool hasTextureCoords = Next<bool>(cursor);
        
        Slice<Vertex> verts = Next<Vertex>(cursor, numVerts);
        Slice<s32> indices  = Next<s32>(cursor, numIndices);
        
        mesh.verts   = ArenaPushSlice(&sceneArena, verts);
        mesh.indices = ArenaPushSlice(&sceneArena, indices);
        mesh.handle = R_UploadMesh(mesh.verts, mesh.indices);
    }
    
    return res;
}

R_Texture LoadTexture(const char* path)
{
    String stbImage = {0};
    int width, height, numChannels;
    stbImage.ptr = (char*)stbi_load(path, &width, &height, &numChannels, 0);
    stbImage.len = width * height * numChannels;
    
    R_Texture res = R_UploadTexture(stbImage, width, height, numChannels);
    stbi_image_free((void*)stbImage.ptr);
    return res;
}

R_Shader LoadShader(const char* path)
{
    bool success = true;
    String contents = LoadEntireFile(path, &success);
    if(!success)
    {
        Log("Failed to load file '%s'\n", path);
        return {0};
    }
    
    defer { free((void*)contents.ptr); };
    
    char** cursor;
    char* c = (char*)contents.ptr;
    cursor = &c;
    
    String magicBytes = Next(cursor, sizeof("shader")-1);  // Excluding null terminator
    if(magicBytes != "shader")
    {
        Log("Attempted to load file '%s' as a shader, which it is not.", path);
        return {0};
    }
    
    u32 version = Next<u32>(cursor);
    if(version != 0)
    {
        Log("Attempted to load file '%s' as a shader, but its version is unsupported.", path);
        return {0};
    }
    
    char* headerPtr = *cursor;
    ShaderBinaryHeader_v0 header = Next<ShaderBinaryHeader_v0>(cursor);
    
    String dxil        = {.ptr=headerPtr+header.dxil, .len=header.dxilSize};
    String vulkanSpirv = {.ptr=headerPtr+header.vulkanSpirv, .len=header.vulkanSpirvSize};
    String glsl        = {.ptr=headerPtr+header.glsl, .len=header.glslSize};
    return R_CompileShader((ShaderKind)header.shaderKind, dxil, vulkanSpirv, glsl);
}

void LoadAssetBinding(const char* path)
{
    ScratchArena scratch;
    Parser parser = InitParser(path, scratch);
    
    //BindingParseResult result = ParseBinding();
    
    // Do some stuff with the result
}

void UnloadScene()
{
    ArenaFreeAll(&sceneArena);
}
