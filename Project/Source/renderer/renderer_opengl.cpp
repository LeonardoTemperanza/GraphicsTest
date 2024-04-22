
#include "base.h"
#include "renderer_generic.h"
#include "embedded_files.h"

// The structure here should be changed so that
// the renderer operates on large amounts of data
// at a time, because these are all function pointers,
// so it's not very fast to repeatedly call these in a loop

u32 Opengl_ShaderKind(ShaderKind kind)
{
    switch(kind)
    {
        default:                 return GL_VERTEX_SHADER;
        case ShaderKind_Vertex:  return GL_VERTEX_SHADER;
        case ShaderKind_Pixel:   return GL_FRAGMENT_SHADER;
        case ShaderKind_Compute: return GL_COMPUTE_SHADER;
    }
    
    return GL_VERTEX_SHADER;
}

void R_Init()
{
    Renderer* r = &renderer;
    memset(r, 0, sizeof(Renderer));
    
    // Allocate buffers
    constexpr int numBuffers = 3;
    GLuint buffers[numBuffers];
    glCreateBuffers(numBuffers, buffers);
    r->appUbo = buffers[0];
    r->frameUbo = buffers[1];
    r->objUbo = buffers[2];
    static_assert(2 < numBuffers);
    
    //glNamedBufferData(r->appUbo, sizeof(PerAppUniforms), nullptr, GL_DYNAMIC_DRAW);
    glNamedBufferData(r->frameUbo, sizeof(PerFrameUniforms), nullptr, GL_DYNAMIC_DRAW);
    glNamedBufferData(r->objUbo, sizeof(PerObjectUniforms), nullptr, GL_DYNAMIC_DRAW);
    
    // @temp This will be later moved to the asset loader (loading and initializing a shader)
    // Specialize and link SPIR-V shader
    GLint compileStatus = 0;
    r->vertShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderBinary(1, &r->vertShader, GL_SHADER_BINARY_FORMAT_SPIR_V, vertShader, sizeof(vertShader));
    glSpecializeShader(r->vertShader, "main", 0, nullptr, nullptr);
    
    r->fragShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderBinary(1, &r->fragShader, GL_SHADER_BINARY_FORMAT_SPIR_V, fragShader, sizeof(fragShader));
    glSpecializeShader(r->fragShader, "main", 0, nullptr, nullptr);
    
    r->shaderProgram = glCreateProgram();
    glAttachShader(r->shaderProgram, r->vertShader);
    glAttachShader(r->shaderProgram, r->fragShader);
    glLinkProgram(r->shaderProgram);
    
    // TODO: Do uniforms really have to be dynamically queried in opengl 4.6?
    int perFrameBindingPoint = 0;
    GLuint perFrameUniformIdx = glGetUniformBlockIndex(r->shaderProgram, "PerFrame");
    // @hack On my laptop (Redmibook 14 AMD Windows 11) this returns -1
    // for some reason... I guess if this happens just set it to 0
    // and just hope for the best?
    if(perFrameUniformIdx == -1) perFrameUniformIdx = 0;
    
    glBindBufferBase(GL_UNIFORM_BUFFER, perFrameBindingPoint, r->frameUbo);
    glUniformBlockBinding(r->shaderProgram, perFrameUniformIdx, perFrameBindingPoint);
    
    int perObjBindingPoint = 1;
    GLuint perObjUniformIdx = glGetUniformBlockIndex(r->shaderProgram, "PerObj");
    if(perObjUniformIdx == -1) perObjUniformIdx = 1;
    
    glBindBufferBase(GL_UNIFORM_BUFFER, perObjBindingPoint, r->objUbo);
    glUniformBlockBinding(r->shaderProgram, perObjUniformIdx, perObjBindingPoint);
}

void R_BeginPass(RenderSettings settings)
{
    Renderer* r = &renderer;
    
    int width, height;
    OS_GetClientAreaSize(&width, &height);
    glViewport(0, 0, width, height);
    float aspectRatio = (float)width / height;
    
    const float n   = settings.nearClipPlane;
    const float f   = settings.farClipPlane;
    const float fov = settings.horizontalFOV;
    
    const float right = n * tan(fov / 2.0f);
    const float top   = right / aspectRatio;
    
    Transform camera = settings.camera;
    
    PerFrameUniforms u;
    u.world2View = transpose(World2ViewMatrix(camera.position, camera.rotation));
    u.view2Proj  = View2ProjMatrix(settings.nearClipPlane, settings.farClipPlane, settings.horizontalFOV, aspectRatio);
    // Negate z axis because in OpenGL, -z is forward while in our coordinate system it's the other way around
    u.view2Proj.c3 *= -1;
    u.view2Proj.m43 = 1;
    u.view2Proj = transpose(u.view2Proj);
    u.viewPos   = camera.position;
    glNamedBufferSubData(r->frameUbo, 0, sizeof(u), &u);
    
    // Preparing render
    glClearColor(0.12f, 0.3f, 0.25f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
}

void R_DrawModelNoReload(Model* model, Vec3 pos, Quat rot, Vec3 scale)
{
    if(!model) return;
    
    Renderer* r = &renderer;
    
    // Scale, rotation and then position
    PerObjectUniforms objUniforms = {0};
    objUniforms.model2World = transpose(Model2WorldMatrix(pos, rot, scale));
    
    glNamedBufferSubData(r->objUbo, 0, sizeof(objUniforms), &objUniforms);
    
    for(int i = 0; i < model->meshes.len; ++i)
    {
        auto& mesh = model->meshes[i];
        
        // shaderProgram should be a material's property
        glUseProgram(model->program);
        glBindVertexArray(mesh.handle);
        glDrawElements(GL_TRIANGLES, mesh.indices.len, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
}

R_Buffer R_UploadMesh(Slice<Vertex> verts, Slice<s32> indices)
{
    GLuint vao;
    glCreateVertexArrays(1, &vao);
    GLuint vbo, ebo;
    constexpr int numBufs = 2;
    GLuint bufferIds[numBufs];
    glCreateBuffers(2, bufferIds);
    vbo = bufferIds[0];
    ebo = bufferIds[1];
    assert(1 < numBufs);
    
    glNamedBufferData(vbo, verts.len * sizeof(verts[0]), verts.ptr, GL_STATIC_DRAW);
    glNamedBufferData(ebo, indices.len * sizeof(indices[0]), indices.ptr, GL_STATIC_DRAW);
    
    // Position
    glEnableVertexArrayAttrib(vao, 0);
    glVertexArrayAttribBinding(vao, 0, 0);
    glVertexArrayAttribFormat(vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
    
    // Normal
    glEnableVertexArrayAttrib(vao, 1);
    glVertexArrayAttribBinding(vao, 1, 0);
    glVertexArrayAttribFormat(vao, 1, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, normal));
    
    // Texture coords
    glEnableVertexArrayAttrib(vao, 2);
    glVertexArrayAttribBinding(vao, 2, 0);
    glVertexArrayAttribFormat(vao, 2, 2, GL_FLOAT, GL_FALSE, offsetof(Vertex, texCoord));
    
    // Tangents
    glEnableVertexArrayAttrib(vao, 3);
    glVertexArrayAttribBinding(vao, 3, 0);
    glVertexArrayAttribFormat(vao, 3, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, tangent));
    
    glVertexArrayVertexBuffer(vao, 0, vbo, 0, sizeof(verts[0]));
    glVertexArrayElementBuffer(vao, ebo);
    return vao;
}

R_Texture R_UploadTexture(String blob, u32 width, u32 height, u8 numChannels)
{
    assert(numChannels >= 2 && numChannels <= 4);
    
    GLuint texture;
    glGenTextures(1, &texture);
    
    // TODO: Use bindless API?
    glBindTexture(GL_TEXTURE_2D, texture);
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        // Maybe an enum in renderer_generic.h for texture format would be cool
        // numChannels is not quite enough
        
        GLint format = GL_RGB;
        switch(numChannels)
        {
            default: format = GL_RGB;  break;
            case 2:  format = GL_RG;   break;
            case 3:  format = GL_RGB;  break;
            case 4:  format = GL_RGBA; break;
        }
        
        // TODO what is internal format vs format?
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, blob.ptr);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    
    return texture;
}

R_Shader R_CompileShader(ShaderKind kind, String dxil, String vulkanSpirv, String glsl)
{
    GLuint shader = glCreateShader(Opengl_ShaderKind(kind));
    GLint glslLen = glsl.len;
    glShaderSource(shader, 1, &glsl.ptr, &glslLen);
    glCompileShader(shader);
    
    GLint status = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if(!status)
    {
        char info[512];
        glGetShaderInfoLog(shader, 512, NULL, info);
        OS_DebugMessage(info);
    }
    
    return shader;
}

R_Program R_LinkShaders(Slice<R_Shader> shaders)
{
    R_Program program = glCreateProgram();
    for(int i = 0; i < shaders.len; ++i)
        glAttachShader(program, shaders[i]);
    
    glLinkProgram(program);
    
    GLint status = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if(!status)
    {
        char info[512];
        glGetProgramInfoLog(program, 512, NULL, info);
        OS_DebugMessage(info);
    }
    
    return program;
}

void R_Cleanup()
{
    
}
