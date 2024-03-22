
#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec3 aTextureCoord;

layout (location = 0) out vec3 fragNormal;
layout (location = 1) out vec3 fragTextureCoord;

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

layout(std140, binding = 1) uniform PerObj
{
    mat4 model2World;
};

void main()
{
    vec4 viewPos = world2View * (model2World * vec4(aPos.x, aPos.y, aPos.z, 1.0f));
    gl_Position = view2Proj * viewPos;
    
    fragNormal = aNormal;
    fragTextureCoord = aTextureCoord;
}
