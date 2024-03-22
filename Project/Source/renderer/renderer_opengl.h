
#pragma once

struct Arena;
union Renderer;

struct gl_Renderer
{
    GLuint appUbo;
    GLuint frameUbo;
    GLuint objUbo;
    GLuint vertShader;
    GLuint fragShader;
    GLuint shaderProgram;
};

struct PerFrameUniforms
{
    Mat4 world2View;
    Mat4 view2Proj;
};

struct PerObjectUniforms
{
    Mat4 model2World;
};

struct gl_MeshInfo
{
    GLuint vao;
    GLuint vbo;
    GLuint ebo;
};

struct gl_TextureInfo
{
    GLuint objId;
};

void gl_RenderModel(Model* model, Vec3 pos, Quat rot, Vec3 scale);

// @cleanup Is this a good idea?
InitRenderer_Signature(gl_InitRenderer);
Render_Signature(gl_Render);
SetupGPUResources_Signature(gl_SetupGPUResources);
Cleanup_Signature(gl_Cleanup);

// For syntax highlighting
#if 0
void gl_InitRenderer();
void gl_Render();
void gl_SetupGPUResources();
void gl_Cleanup();
#endif
