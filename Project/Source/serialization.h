
#pragma once

// This file serves as a bridge for different modules/programs
// to use the same binary structures. The various asset importers
// and the in engine loader share this file

// Shaders

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

// NOTE: Shaders are serialized using these enum
// values, so already existing ones should not be changed
// (Count can and should be changed of course)
enum UniformType
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

struct ShaderBinaryHeader_v0
{
    // Metadata on the shader itself
    u8 shaderKind;
    
    // NOTE: These 3 are actually unused and should be removed
    u32 numMatConstants;
    u32 matNames;
    u32 matOffsets;
    
    // All of these are byte offsets from the address of this struct
    u32 dxil;
    u32 dxilSize;
    u32 vulkanSpirv;
    u32 vulkanSpirvSize;
    u32 glsl;
    u32 glslSize;
};

struct ShaderBinaryHeader_v1
{
    ShaderBinaryHeader_v0 v0;
    
    u32 d3d11Bytecode;
    u32 d3d11BytecodeSize;
};

typedef ShaderBinaryHeader_v1 ShaderBinaryHeader;

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
