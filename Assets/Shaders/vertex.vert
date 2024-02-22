
#version 460 core

layout (location = 0) in vec3 aPos;

#if 0
layout(std140, binding = 0) uniform PerApplication
{
};
#endif

layout(std140, binding = 0) uniform PerFrame
{
    mat4 world2View;
    mat4 view2Proj;
};

void main()
{
    vec4 viewPos = world2View * vec4(aPos.x, aPos.y, aPos.z, 1.0f);
    gl_Position = view2Proj * viewPos;
}