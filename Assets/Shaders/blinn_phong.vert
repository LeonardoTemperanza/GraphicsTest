
#version 460 core

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inTangent;

layout(location = 0) out vec3 worldPos;
layout(location = 1) out vec3 worldNormal;
layout(location = 2) out vec2 texCoord;
layout(location = 3) out mat3 TSToWorld;

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

void main()
{
    vec3 lightDir = normalize(vec3(0.1f, 0.1f, -1.0f));
    
    vec3 viewPos = (world2View * (model2World * vec4(inPos, 1.0f))).xyz;
    
    mat3 normalMatrix = mat3(transpose(inverse(model2World)));
    
    worldPos = (model2World * vec4(inPos, 1.0f)).xyz;
    worldNormal = normalize(normalMatrix * inNormal);
    texCoord = inTexCoord;
    
    vec3 tangent = normalize(normalMatrix * inTangent);
    
    // Reorthogonalize tangent for better quality
    tangent = normalize(tangent - dot(tangent, worldNormal) * worldNormal);
    vec3 bitangent = cross(worldNormal, tangent);
    
    // Convert to tangent space
    TSToWorld = mat3(tangent, bitangent, worldNormal);
    
    gl_Position = view2Proj * vec4(viewPos, 1.0f);
}
