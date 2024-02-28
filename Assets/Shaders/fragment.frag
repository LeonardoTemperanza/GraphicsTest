
#version 460 core

layout (location = 0) in vec3 fragNormal;
layout (location = 0) out vec4 fragColor;

void main()
{
    vec3 objectColor = vec3(1.0f, 0.5f, 0.31f);
    vec3 lightColor = vec3(1.0f, 1.0f, 1.0f);
    vec3 lightDir = normalize(vec3(-0.1f, -0.1f, 1.0f));
    float ambientStrength = 0.01f;
    
    vec3 normal = normalize(fragNormal);
    float diff = max(dot(normal, lightDir), 0.0f);
    vec3 diffuse = diff * lightColor;
    
    vec3 ambient = ambientStrength * lightColor;
    vec3 res = (ambient + diffuse) * objectColor;
    fragColor = vec4(res, 1.0f);
}