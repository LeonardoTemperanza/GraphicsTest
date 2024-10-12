
#pragma vs main

#include "common.hlsli"

struct Vert2Pixel
{
    float4 viewPos   : SV_POSITION;
    
    float3 worldPos  : POSITION;
    float3 normal    : NORMAL;
    float2 uv        : TEXCOORD0;
    float3 tangent   : TANGENT;
};

Vert2Pixel main(Vertex vert)
{
    Vert2Pixel output;
    output.viewPos   = mul(mul(mul(float4(vert.position, 1.0), model2World), world2View), view2Proj);
    output.worldPos  = (float3)mul(float4(vert.position, 1.0), model2World);
    output.normal    = normalize(mul(vert.normal, normalMat));
    output.uv        = vert.uv;
    output.tangent   = normalize((float3)(mul(vert.tangent, normalMat)));
    
    // Orthogonalize the tangent with respect to normal
    output.tangent = normalize(output.tangent - output.normal * dot(output.tangent, output.normal));
    return output;
}
