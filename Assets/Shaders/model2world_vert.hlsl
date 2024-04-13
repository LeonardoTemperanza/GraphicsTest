
#pragma vs VertMain

struct VertexInfo
{
    float3 position : POSITION;
    float2 uv       : TEXCOORD0;
    float3 color    : COLOR;
}

struct Vert2Pixel
{
    float4 position : SV_POSITION;
    float3 uv       : TEXCOORD0;
    float3 color    : TEXCOORD1;
}

// I guess these are uniforms

sampler2D texture;
float2 UVTile;
matrix4x4 worldViewProjection;

Vert2Pixel VertMain(VertexInfo input)
{
    Vert2Pixel output;
    output.position = mul(worldViewProjection, float4(input.position, 1.0));
    output.uv       = input.uv * UVTile;
    output.color    = input.color;
    return output;
}