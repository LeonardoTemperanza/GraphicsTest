
#pragma vs main

// TODO: These should just be in a file that is included in
// every shader

struct VertexInfo
{
    float3 position : POSITION;
    float3 normal   : NORMAL;
    float2 uv       : TEXCOORD0;
    float3 tangent  : TANGENT;
};

struct Vert2Pixel
{
    float4 position : SV_POSITION;
    float3 color    : COLOR;
};

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

Vert2Pixel main(VertexInfo input)
{
    Vert2Pixel output;
    output.position = mul(mul(mul(float4(input.position, 1.0), model2World), world2View), view2Proj);
    output.color    = input.normal;
    return output;
}
