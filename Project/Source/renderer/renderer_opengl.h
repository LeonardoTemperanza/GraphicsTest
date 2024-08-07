
#pragma once

// GPU Resources
typedef u32 R_Texture;
typedef u32 R_Buffer;

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
    // NOTE: Since we're translating from
    // hlsl, all global variables are put inside
    // a "_Globals" uniform block
    Slice<u32> uniformOffsets;
    
    u32 globalsBinding;
    
    Slice<gl_UniformBlock> blocks;
};

struct Renderer
{
    R_Pipeline boundPipeline;
    
    // Objects for simple rendering
    GLuint fullscreenQuad;
};
