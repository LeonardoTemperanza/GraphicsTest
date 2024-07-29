
#pragma vs VertMain

float4x4 transform;

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
    
    return output;
}