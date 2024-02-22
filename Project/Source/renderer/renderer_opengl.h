
#pragma once

#include "renderer_generic.h"

struct gl_Renderer
{
    GLuint vao;
    GLuint vbo;
    GLuint ebo;
    GLuint appUbo;
    GLuint frameUbo;
    GLuint perAppUniformIdx;
    GLuint perFrameUniformIdx;
    GLuint vertShader;
    GLuint fragShader;
    GLuint shaderProgram;
    
    Primitives primitives;
};

struct PerFrameUniforms
{
    Mat4 world2View;
    Mat4 view2Proj;
};

gl_Renderer gl_InitRenderer(Arena* permArena);
void gl_Render(gl_Renderer* r, RenderSettings settings);