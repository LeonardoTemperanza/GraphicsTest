
// NOTE: This is very much a work in progress for a scene based
// (loading screen based) asset management system.
// Renderer can be refactored later without changing this too much

#pragma once

#include "base.h"

struct Asset
{
    const char* name;  // Used for hot reloading
    int id;
};

struct Shader
{
    Asset asset;
    Slice<uchar> blob;  // Depending on the renderer, could be binary or textual
};

struct Texture
{
    Asset asset;
    bool allocatedOnGPU;
    
    s32 width;
    s32 height;
    s32 numChannels;
    
    const char* format;
    String blob;
    
    // API dependent info here
    void* gfxInfo;
};

struct Material
{
    Asset asset;
    
    Slice<Texture*> textures;
    
    Shader* shader;
};

// Vertex data might vary depending on the usage
struct Vertex_v0
{
    Vec3 pos;
    Vec3 normal;
    Vec2 texCoord;
    Vec3 tangent;
    Vec3 bitangent;
};

typedef Vertex_v0 Vertex;

struct Mesh
{
    bool isCPUStorageLoaded;  // CPU storage might be freed right after copying to GPU, we'll see.
    bool hasTextureCoords;
    
    Slice<Vertex> verts;
    Slice<s32> indices;
    
    // API dependent info here
    void* gfxInfo;
    
    Material* material;
};

struct Model
{
    Asset asset;
    Slice<Mesh> meshes;
};

Model* LoadModelAsset(const char* path);
Material* LoadMaterialAsset(const char* path);
Texture* LoadTextureAsset(const char* path);
