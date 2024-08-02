
struct R_Buffer
{
    String name;
    
    R_BufferFlags flags;
    u32 handle;
};

struct R_UniformBuffer
{
    String name;
    
    R_ShaderType stage;
    u32 handle;
};

typedef R_UniformBuffer* R_UniformBufferHandle

struct R_Pipeline
{
    
};