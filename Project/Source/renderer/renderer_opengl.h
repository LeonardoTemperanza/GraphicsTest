
#pragma once

struct Renderer
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
    Vec3 viewPos;
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
