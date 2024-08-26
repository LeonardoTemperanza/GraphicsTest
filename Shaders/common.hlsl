
// Provides the common structures used in most shaders.
// NOTE: This file is heavily bound to the renderer implementation
// and so should mostly not be touched by non-programmers

#pragma header

cbuffer PerScene : register(b0)
{
    
};

// TODO: PerView would probably be a better name
cbuffer PerFrame : register(b1)
{
    float4x4 world2View;
    float4x4 view2Proj;
    float3 viewPos;
};

cbuffer PerObj : register(b2)
{
    float4x4 model2World;
};

struct Vertex
{
    float3 position : POSITION;
    float3 normal   : NORMAL;
    float2 uv       : TEXCOORD0;
    float3 tangent  : TANGENT;
};

#define MaxBonesInfluence 5
#define MaxBones 200 // Maximum number of bones in a skinned mesh
struct SkinnedVertex
{
    float3 position : POSITION;
    float3 normal   : NORMAL;
    float2 uv       : TEXCOORD0;
    float3 tangent  : TANGENT;
    float blendWeights[MaxBonesInfluence] : BLENDWEIGHT;
    uint  blendIndices[MaxBonesInfluence] : BLENDINDICES;
};
