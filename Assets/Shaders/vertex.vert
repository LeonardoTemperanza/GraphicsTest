
#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

layout (location = 0) out vec3 fragNormal;

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
    
    DirectionalLight dirLight;
};

void main()
{
    vec4 viewPos = world2View * vec4(aPos.x, aPos.y, aPos.z, 1.0f);
    gl_Position = view2Proj * viewPos;
    fragNormal = aNormal;
}
