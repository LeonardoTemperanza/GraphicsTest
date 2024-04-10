
#include "asset_system.h"

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

Model* LoadModelAsset(const char* path)
{
    ScratchArena scratch;
    
    auto res = ArenaZAllocTyped(Model, &sceneArena);
    
    FILE* modelFile = fopen(path, "rb");
    // TODO: Error reporting
    if(!modelFile)
    {
        OS_DebugMessage("File not found.\n");
        return res;
    }
    
    defer { fclose(modelFile); };
    
    String contents = LoadEntireFile(path);
    if(!contents.ptr)
    {
        OS_DebugMessage("Could not even load file...\n");
        return res;
    }
    
    defer { free((void*)contents.ptr); };
    
    char** cursor;
    {
        char* c = (char*)contents.ptr;
        cursor = &c;
    }
    
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
        mesh.isCPUStorageLoaded = true;
        //mesh.material = materials[materialIdx];
    }
    
    SetupGPUResources(res, &sceneArena);
    
    return res;
}

Material LoadMaterialAsset(const char* path)
{
    ScratchArena scratch;
    
    Material mat = {0};
    
    FILE* matFile = fopen(path, "rb");
    // TODO: Error reporting
    if(!matFile)
    {
        OS_DebugMessage("File not found.\n");
        return mat;
    }
    
    defer { fclose(matFile); };
    
    String contents = LoadEntireFile(path);
    if(!contents.ptr)
    {
        OS_DebugMessage("Could not even load file...\n");
        return mat;
    }
    
    defer { free((void*)contents.ptr); };
    
    char** cursor;
    {
        char* c = (char*)contents.ptr;
        cursor = &c;
    }
    
    String magicBytes = Next(cursor, sizeof("material")-1);  // Excluding null terminator
    // TODO: Error reporting
    if(magicBytes != "material")
    {
        OS_DebugMessage("This thing is not even a material...\n");
        return mat;
    }
    
    return mat;
}

Texture* LoadTextureAsset(const char* path)
{
    Texture* texture = ArenaZAllocTyped(Texture, &sceneArena);
    
    String stbImage = {0};
    stbImage.ptr = (char*)stbi_load(path, &texture->width, &texture->height, &texture->numChannels, 0);
    stbImage.len = texture->width * texture->height * texture->numChannels;  // 1 byte for each channel and pixel in the image
    
    // Copy back to the arena and free the buffer that was allocated by stb_image
    // @performance Is there something better we could do here?
    // Maybe modify stb_image.h ?
    texture->blob = ArenaPushString(&sceneArena, stbImage);
    stbi_image_free((void*)stbImage.ptr);
    return texture;
}

void ReloadGPUResources()
{
    // Loop through all assets which need to reallocate their GPU resources
    TODO;
}

void UnloadScene()
{
    ArenaFreeAll(&sceneArena);
}

void SetMaterial(Model* model, Material material, int idx)
{
    assert(idx < model->materials.len);
    model->materials[idx] = material;
}
