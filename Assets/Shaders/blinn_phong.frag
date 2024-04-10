
#version 460 core

layout(location = 0) in vec3 worldPos;
layout(location = 1) in vec3 worldNormal;
layout(location = 2) in vec2 texCoord;
layout(location = 3) in mat3 TSToWorld;

layout(location = 0) out vec4 fragColor;

struct DirectionalLight
{
    vec3 direction;
    
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

layout(std140, binding = 0) uniform PerFrame
{
    mat4 world2View;
    mat4 view2Proj;
    vec3 viewPos;
};

layout(std140, binding = 1) uniform PerObj
{
    mat4 model2World;
};

layout(binding = 0) uniform sampler2D diffuseMap;
layout(binding = 1) uniform sampler2D normalMap;

void main()
{
    // Get normal from normal map
    vec3 normalMapSample = texture(normalMap, texCoord).rgb * 2.0f - 1.0f;
    vec3 transNormal = TSToWorld * normalMapSample;
    
    vec3 lightColor = vec3(0.8f, 0.6f, 1.0f);
    vec3 lightDir = normalize(-vec3(-1.0f, -0.1f, -0.1f));
    
    vec3 color = texture(diffuseMap, texCoord).rgb;
    
    transNormal = normalize(worldNormal);  // For debugging
    
    vec3 viewDir = normalize(viewPos - worldPos);
    
    // Diffuse shading
    float diff = max(dot(transNormal, lightDir), 0.0f);
    
    // Specular shading
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(transNormal, halfwayDir), 0.0f), 2.0f);
    
    // Smoothly remove specular highlights from backfacing triangles
    float nDotL = dot(lightDir, transNormal);
    spec *= smoothstep(0.0f, 1.0f, nDotL);
    
    // Combine results
    vec3 ambient  = 0.2f * color;
    vec3 diffuse  = 0.7f * diff * color;
    vec3 specular = 0.2f * spec * vec3(1.0f);
    
    //fragColor = vec4(ambient + diffuse + specular, 1.0f);
    fragColor = vec4(transNormal, 1.0f);
}
