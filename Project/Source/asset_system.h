
// NOTE: This is very much a work in progress for a scene based
// (loading screen based) asset management system.

#pragma once

#include "base.h"

typedef u32 GfxObjId;

struct Asset
{
    const char* name;  // Used for hot reloading
    int id;
};

struct Shader
{
    Asset asset;
    Slice<uchar> blob;  // Depending on the renderer could be binary or textual
};

struct Texture
{
    Asset asset;
    
    const char* format;
    Slice<uchar> blob;
};

struct Material
{
    Asset asset;
    
    // Textures and stuff like that...
    
    Shader* shader;
};

struct Mesh
{
    bool isCPUStorageLoaded;  // CPU storage might be freed right after copying to GPU, we'll see.
    bool hasTextureCoords;
    
    Slice<Vec3> verts;
    Slice<Vec3> normals;
    Slice<Vec3> textureCoords;
    Slice<s32>  indices;
    
    // Some union here for the various graphics APIs?
    
    Material* material;
};

struct Model
{
    Asset asset;
    Slice<Mesh> meshes;
};

Model* LoadModelAsset(const char* path, Arena* dst);
