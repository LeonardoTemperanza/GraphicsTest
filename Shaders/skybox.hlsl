
#pragma vs vertMain
#pragma ps pixelMain

// 

struct VertexInfo
{
    float3 position : POSITION;
    float3 normal   : NORMAL;
    float2 uv       : TEXCOORD0;
    float3 tangent  : TANGENT;
};

struct Vert2Pixel
{
    float4 position  : SV_POSITION;
    float3 texCoords : TEXCOORD0;
};


// TODO: These should just be in a file that is included in
// every shader
cbuffer PerScene : register(b0)
{
    
};

cbuffer PerFrame : register(b1)
{
    column_major float4x4 world2View;
    column_major float4x4 view2Proj;
    float3 viewPos;
};

cbuffer PerObj : register(b2)
{
    column_major float4x4 model2World;
};

Vert2Pixel vertMain(VertexInfo input)
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