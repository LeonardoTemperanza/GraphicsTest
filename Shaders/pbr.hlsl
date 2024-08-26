
#pragma ps pixelMain

#include "common.hlsl"

struct Vert2Pixel
{
    float4 position : SV_POSITION;
    float3 normal   : NORMAL;
    float2 uv       : TEXCOORD0;
    float3 tangent  : TANGENT;
};

float4 pixelMain(Vert2Pixel input) : SV_TARGET
{
    return float4(input.normal, 1.0f);
}
