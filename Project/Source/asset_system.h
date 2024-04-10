
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

// This can be a plain text file which 
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
};

typedef Vertex_v0 Vertex;

#define MaxBonesInfluence
struct AnimVert_v0
{
    Vec3 pos;
    Vec3 normal;
    Vec2 texCoord;
    Vec3 tangent;
    
    struct BoneWeight
    {
        int boneIdx;
        float weight;
    } boneWeights[MaxBonesInfluence];
};

typedef AnimVert_v0 AnimVert;

struct Mesh
{
    bool isCPUStorageLoaded;
    bool hasTextureCoords;
    bool hasBones;
    
    Slice<Vertex> verts;
    Slice<s32> indices;
    
    // API dependent info here
    void* gfxInfo;
    
    int materialIdx;
};

struct Model
{
    Asset asset;
    Slice<Mesh> meshes;
    Slice<Material> materials;
};

Model* LoadModelAsset(const char* path);
Material LoadMaterialAsset(const char* path);
Texture* LoadTextureAsset(const char* path);
void SetMaterial(Model* model, Material* material, int idx);
