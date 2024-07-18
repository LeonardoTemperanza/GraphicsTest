
#pragma vs vertMain
#pragma ps pixelMain

// Should this stuff automatically
// be inserted into the shader with a
// pragma perhaps (which tells the renderer
// what configuration to use)? Except
// for the material stuff.

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

cbuffer PerScene
{
    
};

cbuffer PerFrame
{
    column_major float4x4 world2View;
    column_major float4x4 view2Proj;
    float3 viewPos;
};

cbuffer PerObj
{
    column_major float4x4 model2World;
};

cbuffer Material
{
    float shininess;
}

Vert2Pixel vertMain(VertexInfo input)
{
    Vert2Pixel output;
    output.position = mul(mul(mul(float4(input.position, 1.0), model2World), world2View), view2Proj);
    output.color    = input.normal;
    return output;
}

float4 pixelMain(Vert2Pixel input) : SV_TARGET
{
    return float4(input.color, 1.0f);
}