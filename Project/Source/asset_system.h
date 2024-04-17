
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

struct ConstantBufferFormat
{
    u32 size;
    Slice<String> names;
    Slice<u32>    offsets;
};

// NOTE: Shaders are serialized using these enum
// values, so already existing ones should not be changed
// (Count can be changed of course, we do not serialize it)
enum ShaderKind
{
    ShaderKind_None    = 0,
    ShaderKind_Vertex  = 1,
    ShaderKind_Pixel   = 2,
    ShaderKind_Compute = 3,
    ShaderKind_Count,
};

struct Shader
{
    Asset asset;
    ShaderKind kind;
    
    ConstantBufferFormat materialFormat;
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

struct Vertex_v0
{
    Vec3 pos;
    Vec3 normal;
    Vec2 texCoord;
    Vec3 tangent;
};

typedef Vertex_v0 Vertex;

#define MaxBonesInfluence 4
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
    Slice<s32>    indices;
    
    // API dependent info here
    void* gfxInfo;
    
    int materialIdx;
};

struct Model
{
    Asset asset;
    Slice<Mesh>     meshes;
    Slice<Material> materials;
};

Model* LoadModelAsset(const char* path);
Material LoadMaterialAsset(const char* path);
Texture* LoadTextureAsset(const char* path);
void SetMaterial(Model* model, Material* material, int idx);
