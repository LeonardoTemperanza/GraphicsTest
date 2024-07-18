
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
    
    String contents = LoadEntireFile(path);
    if(!contents.ptr)
    {
        OS_DebugMessage("Could not even load file...\n");
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
        OS_DebugMessage("This thing is not even a model file...\n");
        return res;
    }
    
    s32 version        = Next<s32>(cursor);
    s32 numMeshes      = Next<s32>(cursor);
    s32 numMaterials   = Next<s32>(cursor);
    res->meshes.ptr    = ArenaZAllocArray(Mesh, numMeshes, &sceneArena);
    res->meshes.len    = numMeshes;
    res->materials.ptr = ArenaZAllocArray(Material, numMaterials, &sceneArena);
    res->materials.len = numMaterials;
    
    char buffer[1024];
    snprintf(buffer, 1024, "Version: %d, Num meshes: %d, Num materials: %d\n", version, numMeshes, numMaterials);
    OS_DebugMessage(buffer);
    
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

Material LoadMaterial(const char* path)
{
    ScratchArena scratch;
    
    Material mat = {0};
    
    String contents = LoadEntireFile(path);
    if(!contents.ptr)
    {
        OS_DebugMessage("Could not load file...\n");
        return mat;
    }
    
    defer { free((void*)contents.ptr); };
    
    char** cursor;
    char* c = (char*)contents.ptr;
    cursor = &c;
    
    String magicBytes = Next(cursor, sizeof("material")-1);  // Excluding null terminator
    // TODO: Error reporting
    if(magicBytes != "material")
    {
        OS_DebugMessage("This file is corrupt or does not contain material\n");
        return mat;
    }
    
    return mat;
}

Texture* LoadTexture(const char* path)
{
    Texture* texture = ArenaZAllocTyped(Texture, &sceneArena);
    
    String stbImage = {0};
    stbImage.ptr = (char*)stbi_load(path, &texture->width, &texture->height, &texture->numChannels, 0);
    stbImage.len = texture->width * texture->height * texture->numChannels;
    
    texture->handle = R_UploadTexture(stbImage, texture->width, texture->height, texture->numChannels);
    stbi_image_free((void*)stbImage.ptr);
    return texture;
}

Shader* LoadShader(const char* path)
{
    Shader* shader = ArenaZAllocTyped(Shader, &sceneArena);
    
    String contents = LoadEntireFile(path);
    if(!contents.ptr)
    {
        OS_DebugMessage("Could not load file...\n");
        return shader;
    }
    
    defer { free((void*)contents.ptr); };
    
    char** cursor;
    char* c = (char*)contents.ptr;
    cursor = &c;
    
    String magicBytes = Next(cursor, sizeof("shader")-1);  // Excluding null terminator
    if(magicBytes != "shader")
    {
        OS_DebugMessage("This file is corrupt or does not contain shader\n");
        return shader;
    }
    
    u32 version = Next<u32>(cursor);
    if(version != 0)
    {
        OS_DebugMessage("Wrong shader version\n");
        return shader;
    }
    
    char* headerPtr = *cursor;
    ShaderBinaryHeader_v0 header = Next<ShaderBinaryHeader_v0>(cursor);
    
    String dxil        = {.ptr=headerPtr+header.dxil, .len=header.dxilSize};
    String vulkanSpirv = {.ptr=headerPtr+header.vulkanSpirv, .len=header.vulkanSpirvSize};
    String glsl        = {.ptr=headerPtr+header.glsl, .len=header.glslSize};
    shader->handle = R_CompileShader((ShaderKind)header.shaderKind, dxil, vulkanSpirv, glsl);
    
    return shader;
}

void LoadAssetBinding(const char* path)
{
    ScratchArena scratch;
    Parser parser = InitParser(path, scratch);
    
    //BindingParseResult result = ParseBinding();
    
    // Do some stuff with the result
}

void SetMaterial(Model* model, Material material, int idx)
{
    assert(idx < model->materials.len);
    model->materials[idx] = material;
}

void UnloadScene()
{
    ArenaFreeAll(&sceneArena);
}
