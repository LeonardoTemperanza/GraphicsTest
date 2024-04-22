
// NOTE: This is very much a work in progress for a scene based
// (loading screen based) asset management system.
// Renderer can be refactored later without changing this too much

#pragma once

#include "base.h"
#include "renderer/renderer_generic.h"

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
// (Count can and should be changed of course)
enum ShaderKind
{
    ShaderKind_None    = 0,
    ShaderKind_Vertex  = 1,
    ShaderKind_Pixel   = 2,
    ShaderKind_Compute = 3,
    ShaderKind_Count,
};

struct ShaderBinaryHeader_v0
{
    // Metadata on the shader itself
    u8 shaderKind;
    
    // All of these are byte offsets from the address of this struct
    
    u32 numMatConstants;
    u32 matNames;
    u32 matOffsets;
    
    u32 dxil;
    u32 dxilSize;
    u32 vulkanSpirv;
    u32 vulkanSpirvSize;
    u32 glsl;
    u32 glslSize;
};

typedef ShaderBinaryHeader_v0 ShaderBinaryHeader;

struct Shader
{
    Asset asset;
    ShaderKind kind;
    
    ConstantBufferFormat perFrameFormat;
    ConstantBufferFormat sceneFormat;
    ConstantBufferFormat materialFormat;
    Slice<uchar> blob;  // Depending on the renderer, could be binary or textual
    
    R_Shader handle;
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
    
    // @tmp testing
    Shader* vertex;
    Shader* pixel;
    R_Program program;
};

inline const char* GetShaderKindString(ShaderKind kind)
{
    switch(kind)
    {
        default:                 return "unknown";
        case ShaderKind_Vertex:  return "vertex";
        case ShaderKind_Pixel:   return "pixel";
        case ShaderKind_Compute: return "compute";
    }
    
    return "unknown";
}

void LoadScene(const char* path);
Model* LoadModelAsset(const char* path);
Material LoadMaterialAsset(const char* path);
Texture* LoadTextureAsset(const char* path);
Shader* LoadShader(const char* path);

void SetMaterial(Model* model, Material* material, int idx);

void UnloadScene(const char* path);
