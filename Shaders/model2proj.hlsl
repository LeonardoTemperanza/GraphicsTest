
#pragma vs main

#include "common.hlsl"

struct Vert2Pixel
{
    float4 position  : SV_POSITION;
    float3 normal    : NORMAL;
    float2 uv        : TEXCOORD0;
    float3 tangent   : TANGENT;
};

Vert2Pixel main(Vertex vert)
{
    Vert2Pixel output;
    output.position  = mul(mul(mul(float4(vert.position, 1.0), model2World), world2View), view2Proj);
    output.normal    = vert.normal;
    output.uv        = vert.uv;
    output.tangent   = vert.tangent;
    return output;
}
