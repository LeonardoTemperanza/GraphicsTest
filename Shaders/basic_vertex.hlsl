
#pragma vs VertMain


cbuffer Uniforms : register(b1)
{
    float4x4 transform;
    float f;
    float f2;
};

float4 testMul : register(c0);
float4 test : register(c1);

struct VertOutput
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
};

VertOutput VertMain(float3 position : POSITION, float3 normal : NORMAL)
{
    VertOutput output = (VertOutput)0;
    output.position = mul(float4(position, 1.0f), transform);
    output.normal = (float3)0;
    output.position = mul(mul(output.position, test), testMul);
    
    return output;
}