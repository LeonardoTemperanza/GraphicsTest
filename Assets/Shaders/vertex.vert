
#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

layout(std140, binding = 0) uniform PerFrame
{
    mat4 world2View;
    mat4 view2Proj;
};

layout (location = 0) out vec3 fragNormal;

void main()
{
    vec4 viewPos = world2View * vec4(aPos.x, aPos.y, aPos.z, 1.0f);
    gl_Position = view2Proj * viewPos;
    fragNormal = aNormal;
}
