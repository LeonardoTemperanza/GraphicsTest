
#pragma once

// This file serves as a bridge for different modules/programs
// to use the same binary structures. The various asset importers
// and the in engine loader share this file

// Shaders

// NOTE: Shaders are serialized using these enum
// values, so already existing ones should not be changed
// (Count can and should be changed of course)
enum ShaderType
{
    ShaderType_Null    = 0,
    ShaderType_Vertex  = 1,
    ShaderType_Pixel   = 2,
    
    ShaderType_Count,
};

// NOTE: Shaders are serialized using these enum
// values, so already existing ones should not be changed
// (Count can and should be changed of course)
enum ShaderValType
{
    Uniform_None  = 0,
    Uniform_Int   = 1,
    Uniform_UInt  = 2,
    Uniform_Float = 3,
    Uniform_Vec3  = 4,
    Uniform_Vec4  = 5,
    Uniform_Mat4  = 6,
    
    Uniform_Count,
};

// Everything is represented with the byte offsets from the address of this struct
struct ShaderBinaryHeader_v0
{
    // Metadata on the shader itself
    u8 shaderType;
    
    // Material constants
    u32 matConstantsTypes;  // Points to an array of UniformType
    u32 matConstantsTypesCount;
    u32 matTexturesCount;
    
    // Code controlled parameters
    u32 codeConstantsTypes;
    u32 codeConstantsTypesCount;
    u32 codeTexturesCount;
    
    u32 d3d11Bytecode;
    u32 d3d11BytecodeSize;
    u32 dxil;
    u32 dxilSize;
    u32 vulkanSpirv;
    u32 vulkanSpirvSize;
    u32 glsl;
    u32 glslSize;
};

typedef ShaderBinaryHeader_v0 ShaderBinaryHeader;

// Models

struct Vertex
{
    Vec3 pos;
    Vec3 normal;
    Vec2 texCoord;
    Vec3 tangent;
};

// TODO: First draft of what it would look like for
// a skeletal mesh
#define MaxBonesInfluence 4
struct AnimVert
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

inline const char* GetShaderTypeString(ShaderType kind)
{
    switch(kind)
    {
        case ShaderType_Null:   return "null";
        case ShaderType_Count:   return "count";
        case ShaderType_Vertex: return "vertex";
        case ShaderType_Pixel:  return "pixel";
    }
    
    return "unknown";
}

struct MeshHeader_v0
{
    bool isSkinned;
    
    s32 numVerts;
    s32 numIndices;
    bool hasTextureCoords;
    u32 vertsOffset;
    u32 indicesOffset;
};

typedef MeshHeader_v0 MeshHeader;
