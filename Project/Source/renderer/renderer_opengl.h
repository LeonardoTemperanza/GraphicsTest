
#pragma once

struct Arena;
union Renderer;

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
};

struct PerFrameUniforms
{
    Mat4 world2View;
    Mat4 view2Proj;
};

void gl_InitRenderer(Renderer* renderer, Arena* renderArena);
void gl_Render(Renderer* renderer, RenderSettings settings);
