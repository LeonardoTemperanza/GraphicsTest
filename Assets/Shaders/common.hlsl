
// Provides the common structures used in all shaders.
// NOTE: This file is heavily bound to the renderer implementation
// and so should mostly not be touched by non-programmers

#pragma header

// Parameters set in the material system
#define MaterialSlot b3
// Parameters set in code
#define GlobalsSlot b4

// Texture slots

// Slots accessed through code
#define CodeTex0     t0
#define CodeTex1     t1
#define CodeTex2     t2
#define CodeTex3     t3
#define CodeTex4     t4
#define CodeTex5     t5
#define CodeTex6     t6
#define CodeTex7     t7
#define CodeTex8     t8
#define CodeTex9     t9
// Slots accessed through the material system
#define MaterialTex0 t10
#define MaterialTex1 t11
#define MaterialTex2 t12
#define MaterialTex3 t13
#define MaterialTex4 t14
#define MaterialTex5 t15
#define MaterialTex6 t16
#define MaterialTex7 t17
#define MaterialTex8 t18
#define MaterialTex9 t19

// Sampler slots

// Slots accessed through code
#define CodeSampler0     s0
#define CodeSampler1     s1
#define CodeSampler2     s2
#define CodeSampler3     s3
#define CodeSampler4     s4
#define CodeSampler5     s5
#define CodeSampler6     s6
#define CodeSampler7     s7
#define CodeSampler8     s8
#define CodeSampler9     s9
// Slots accessed through the material system
#define MaterialSampler0 s10
#define MaterialSampler1 s11
#define MaterialSampler2 s12
#define MaterialSampler3 s13
#define MaterialSampler4 s14
#define MaterialSampler5 s15
#define MaterialSampler6 s16
#define MaterialSampler7 s17
#define MaterialSampler8 s18
#define MaterialSampler9 s19

// CBuffers
#define PerSceneCBuf b0
#define PerFrameCBuf b1
#define PerObjCBuf   b2

#define CodeCBuffer
#define MaterialConstants 

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
    float3x3 normalMat;
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
