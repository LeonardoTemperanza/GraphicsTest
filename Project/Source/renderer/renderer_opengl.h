
#pragma once

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

struct Renderer
{
    GLuint appUbo;
    GLuint frameUbo;
    GLuint objUbo;
    
    GLuint vertShader;
    GLuint fragShader;
    GLuint shaderProgram;
    
    PerFrameUniforms perFrameUniforms;
    
    R_Shader boundShader;
    
    // Basic shapes
    GLuint basicProgram;
    GLuint colorUniform;
    GLuint transformUniform;
    
    GLuint fullScreenQuadVao;
    GLuint cylinderVbo;
    GLuint coneVbo;
    
    // Mouse picking
    GLuint mousePickingProgram;
    GLuint mousePickingIdUniform;
    GLuint mousePickingColor;
    GLuint mousePickingDepth;
    GLuint mousePickingFbo;
    
    // Selection outlines
    GLuint selectionProgram;
    GLuint selectionColor;
    GLuint selectionDepth;
    GLuint selectionFbo;
    
    GLuint outlineProgram;
    GLuint outlineColorUniform;
    GLuint outlineTransformUniform;
};
