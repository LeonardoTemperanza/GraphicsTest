
#pragma vs vertMain

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

Vert2Pixel vertMain(VertexInfo input)
{
    Vert2Pixel output;
    output.position = float4(input.position, 1.0);
    output.color    = input.normal;
    return output;
}
