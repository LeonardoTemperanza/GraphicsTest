
// A few notes so i don't forget things
// There are string ids which signify intent for a particular asset.
// To reference assets in files those will primarily be used instead of the path,
// so that the path itself can be changed easily. There are "assetbinding" files that
// specify the file path for each string id (or asset intent).
// Models are just binary files with vertex info and others
// Shaders are just binary files with shader binaries (or sources) for the corresponding graphics apis
// Textures are just png files (the simplest files here)
// Materials are the only textual files at the moment (other than asset bindings) which contain references to string ids

// A string map can be used for looking up resources from their string id. Also used for hot reloading.
// Textual files like materials which reference other assets via their string id, will be loaded using the string map, to prevent
// loading the same texture/shader multiple times, wasting space and time.

#pragma once

#include "base.h"
#include "renderer/renderer_generic.h"

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
    String id;
    ShaderKind kind;
    ConstantBufferFormat materialFormat;
    R_Shader handle;
};

struct Texture
{
    String id;
    s32 width;
    s32 height;
    s32 numChannels;
    
    R_Texture handle;
};

struct Material
{
    String id;
    Slice<Texture*> textures;
    Shader* shader;
};

struct Mesh
{
    bool hasTextureCoords;
    bool hasBones;
    
    Slice<Vertex> verts;
    Slice<s32>    indices;
    
    R_Buffer handle;
};

struct Model
{
    String id;
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
Model* LoadModel(const char* path);
Material LoadMaterial(const char* path);
Texture* LoadTexture(const char* path);
Shader* LoadShader(const char* path);
void LoadAssetBinding(const char* path);

void SetMaterial(Model* model, Material* material, int idx);

void UnloadScene(const char* path);
