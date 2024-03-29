
#version 460 core

layout (location = 0) in vec3 fragPos;
layout (location = 1) in vec3 fragNormal;
layout (location = 2) in vec2 fragTextureCoord;
layout (location = 3) in vec3 fragTangent;
layout (location = 4) in vec3 fragBitangent;
layout (location = 5) in vec3 fragViewPos;

layout (location = 0) out vec4 fragColor;

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

layout(binding = 0) uniform sampler2D diffuseTexture;
layout(binding = 1) uniform sampler2D normalsTexture;

void main()
{
    // Get normal from normal map
    vec3 normal = texture(normalsTexture, fragTextureCoord).rgb;
    normal = normalize(normal * 2.0f - 1.0f);  // From range [0, 1] to range [-1, 1]
    
    vec3 lightColor = vec3(0.8f, 0.6f, 1.0f);
    vec3 lightDir = normalize(vec3(0.1f, 0.1f, -1.0f));
    
    vec3 color = texture(diffuseTexture, fragTextureCoord).rgb;
    
    // Ambient
    float ambientStrength = 0.1f;
    vec3 ambient = ambientStrength * color;
    
    // Diffuse 
    float diff = max(dot(normal, lightDir), 0.0f);
    vec3 diffuse = diff * color;
    
    // Specular
    float specularStrength = 0.5f;
    vec3 viewDir = normalize(fragViewPos - fragPos);
    vec3 reflectDir = reflect(-lightDir, normal);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(viewDir, halfwayDir), 0.0f), 32.0f);
    vec3 specular = specularStrength * spec * lightColor;  
    
    vec3 result = ambient + diffuse + specular;
    fragColor = vec4(normal, 1.0f);
}
