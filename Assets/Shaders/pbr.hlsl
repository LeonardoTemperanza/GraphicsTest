
#pragma ps pixelMain

#include "common.hlsli"

struct Vert2Pixel
{
    float4 viewPos   : SV_POSITION;
    float3 worldPos  : POSITION;
    float3 normal    : NORMAL;
    float2 uv        : TEXCOORD0;
    float3 tangent   : TANGENT;
};

Texture2D<float4> diffuseMap : register(t0);
Texture2D<float4> normalMap  : register(t1);
SamplerState linearSampler   : register(s0);

float4 pixelMain(Vert2Pixel input) : SV_TARGET
{
    // Sanitize input
    input.normal     = normalize(input.normal);
    input.tangent    = normalize(input.tangent);
    float3 bitangent = normalize(cross(input.normal, input.tangent));
    
    // Light params
    float3 lightDir = normalize(-float3(20.921f, 7.0f, 17.236f));
    float3 lightColor = float3(1.0f, 1.0f, 1.0f);
    float3 lightAmbientColor = float3(0.3f, 0.3f, 0.3f);
    float3 lightDiffuseColor = float3(0.8f, 0.8, 0.8f);
    float3 lightSpecularColor = float3(0.2f, 0.2f, 0.2f);
    
    // Compute normal
    float3 normalSample = normalize(normalMap.Sample(linearSampler, input.uv).xyz * 2.0f - 1.0f);
    float3x3 tbn = float3x3(input.tangent, bitangent, input.normal);
    float3 normal = mul(normalSample, tbn);
    
    // Sample textures
    float4 diffuseSample = diffuseMap.Sample(linearSampler, input.uv);
    
    float3 viewDir = normalize(viewPos - input.worldPos);
    
    float3 towardsLight = -lightDir;
    
    // Ambient
    float3 ambient = lightAmbientColor * diffuseSample.xyz;
    
    // Diffuse
    float diffuseIntensity = saturate(dot(normal, towardsLight));
    float3 diffuse = diffuseIntensity * lightDiffuseColor * (float3)diffuseSample;
    
    float3 specular = (float3)0.0f;
    if(diffuseIntensity > 0.0f)
    {
        float3 half = normalize(towardsLight + viewDir);
        float specularIntensity = pow(saturate(dot(normal, half)), 10.0f);
        specular = specularIntensity * lightSpecularColor;
    }
    
    float3 final = ambient + diffuse + specular;
    return float4(final, 1.0f);
}
