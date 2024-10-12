
#pragma vs main

#include "common.hlsli"

struct Vertex2Pixel
{
    float4 position  : SV_POSITION;
    float3 normal    : NORMAL;
    float2 uv        : TEXCOORD0;
    float3 tangent   : TANGENT;
};

// @speed This can be a 4x3 matrix (there are lots of bones)
uniform float4x4 boneTransforms[MaxBones];

Vertex2Pixel main(SkinnedVertex vert)
{
    Vertex2Pixel output = (Vertex2Pixel)0;
    
    output.position = (float4)0.0f;
    for(int i = 0; i < MaxBonesInfluence; ++i)
    {
        float4 localPosition = mul(float4(vert.position, 0.0f), boneTransforms[vert.blendIndices[i]]);
        output.position += localPosition * vert.blendWeights[i];
    }
    
    output.position = mul(mul(output.position, world2View), view2Proj);
    output.normal   = vert.normal;
    output.uv       = vert.uv;
    output.tangent  = vert.tangent;
    
    return output;
}
