
#include "asset_system.h"

Model* LoadModel(const char* path, Arena* dst)
{
    auto res = ArenaAllocTyped(Model, dst);
    memset(res, 0, sizeof(*res));
    
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
    
    s32 version      = Next<s32>(cursor);
    s32 numMeshes    = Next<s32>(cursor);
    s32 numMaterials = Next<s32>(cursor);
    res->meshes.ptr  = ArenaAllocArray(Mesh, numMeshes, dst);
    res->meshes.len  = numMeshes;
    
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
        
        Slice<Vec3> verts     = Next<Vec3>(cursor, numVerts);
        Slice<Vec3> normals   = Next<Vec3>(cursor, numVerts);
        Slice<Vec3> texCoords = Next<Vec3>(cursor, numVerts * hasTextureCoords);
        Slice<s32>  indices   = Next<s32>(cursor, numIndices);
        mesh.verts   = ArenaPushSlice(dst, verts);
        mesh.indices = ArenaPushSlice(dst, indices);
        mesh.normals = ArenaPushSlice(dst, normals);
    }
    
    return res;
}