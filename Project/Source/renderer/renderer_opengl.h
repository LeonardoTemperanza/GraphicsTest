
#pragma once

// GPU Resources
//typedef u32 R_Texture;
typedef u32 R_Buffer;

typedef u32 R_Cubemap;

struct R_Texture
{
    GLuint handle;
    R_TextureFormat format;
};

#define gl_FramebufferMaxTextures 8
struct R_Framebuffer
{
    GLuint handle;
    bool color;
    bool depth;
    bool stencil;
    int width;
    int height;
    R_TextureFormat colorFormat;
    
    GLuint textures[gl_FramebufferMaxTextures];
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

struct gl_UniformBlock
{
    Slice<u32> offsets;
    Slice<UniformType> types;
};

struct R_UniformBuffer
{
    u32 buffer;
    u32 binding;
};

struct R_Shader
{
    ShaderKind kind;
    GLuint handle;
};

struct R_Pipeline
{
    Slice<R_Shader> shaders;
    GLuint handle;
    
    u32 globalsUniformBlockIndex;
    bool hasGlobals;
    
    Slice<gl_UniformBlock> blocks;
};

struct Renderer
{
    R_Pipeline boundPipeline;
    R_Framebuffer boundFramebuffer;
    
    // Uniform buffers
    GLuint globalsBuffer;
    GLuint perSceneBuffer;
    GLuint perFrameBuffer;
    GLuint perObjBuffer;
    
    /*
    R_UniformBuffer globalsUniformBuffer;
R_UniformBuffer perSceneUniformBuffer;
R_UniformBuffer perFrameUniformBuffer;
R_UniformBuffer perObjUniformBuffer;
*/
    
    // Objects for simple rendering
    GLuint fullscreenQuad;
    GLuint quadVao;
    GLuint quadVbo;
    GLuint unitCylinder;
    u32 unitCylinderCount;
    GLuint unitCone;
    u32 unitConeCount;
};
