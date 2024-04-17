
#pragma vs vertMain
#pragma ps pixelMain

struct VertexInfo
{
    float3 position : POSITION;
    float2 uv       : TEXCOORD0;
    float3 color    : COLOR;
};

struct Vert2Pixel
{
    float4 position : SV_POSITION;
    float2 uv       : TEXCOORD0;
    float3 color    : TEXCOORD1;
};

cbuffer PerScene
{
    
};

cbuffer PerFrame
{
    float4x4 world2View;
    float4x4 view2Proj;
    float3 viewPos;
};

cbuffer PerObj
{
    float4x4 model2World;
};

// This will contain only the stuff that should be configurable by the material
// system. Stuff like light direction etc. is directly given by the renderer, but
// this stuff is actually configurable. The name material is used.
cbuffer Material
{
    float shininess;
}

// I guess these are uniforms

Vert2Pixel vertMain(VertexInfo input)
{
    // Stuff will be interpolated of course
    Vert2Pixel output;
    output.position = mul(world2View, float4(input.position, 1.0));
    output.uv       = input.uv;
    output.color    = input.color;
    return output;
}

float4 pixelMain(Vert2Pixel input) : SV_TARGET
{
    return float4(input.color, 1.0f);
}