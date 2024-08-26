
#pragma vs vertMain
#pragma ps pixelMain

#include "common.hlsl"

struct Vert2Pixel
{
    float4 position  : SV_POSITION;
    float3 texCoords : TEXCOORD0;
};

Vert2Pixel vertMain(Vertex input)
{
    Vert2Pixel output;
    
    // Ignore the model2world matrix, and don't apply position
    // of the view matrix
    float3 posView = mul(input.position, (float3x3)world2View);
    output.position  = mul(float4(posView, 1.0), view2Proj);
    output.texCoords = input.position;
    return output;
}

TextureCube envMap : register(t0);
SamplerState linearSampler : register(s0);

float4 pixelMain(Vert2Pixel input) : SV_TARGET
{
    return envMap.Sample(linearSampler, normalize(input.texCoords));
}