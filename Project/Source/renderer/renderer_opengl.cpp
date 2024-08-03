
#include "base.h"
#include "renderer_generic.h"
#include "embedded_files.h"

static const Vec3 quadVerts[] =
{
    {0, 0, 0},
    {1, 0, 0},
    {1, 1, 0},
    {0, 0, 0},
    {1, 1, 0},
    {0, 1, 0},
};

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
    
    // TODO: This should actually initialize the opengl
    // context. Right now it resides in the platform layer
    // but i think it would be best if we handled it here,
    // using some #ifdefs with platform specific code in there
}

void R_DrawModelNoReload(Model* model)
{
    if(!model) return;
    Renderer* r = &renderer;
    
    for(int i = 0; i < model->meshes.len; ++i)
    {
        auto& mesh = model->meshes[i];
        
        glBindVertexArray(mesh.handle);
        glDrawElements(GL_TRIANGLES, (GLsizei)mesh.indices.len, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
}

Mat4 R_ConvertView2ProjMatrix(Mat4 mat)
{
    mat.c3 *= -1;
    mat.m43 = 1;
    return mat;
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

R_UniformBuffer R_CreateUniformBuffer(u32 binding)
{
    R_UniformBuffer res = {0};
    glCreateBuffers(1, &res.buffer);
    res.binding = binding;
    glBindBufferBase(GL_UNIFORM_BUFFER, res.binding, res.buffer);
    return res;
}

void R_UploadUniformBuffer(R_UniformBuffer uniformBuffer, Slice<R_UniformValue> desc)
{
    // NOTE: Assuming the uniform buffers are padded with
    // std140!! Changing the standard used (by modifying the
    // shader importer) will make the renderer suddenly incorrect
    ScratchArena scratch;
    Slice<uchar> buffer = MakeUniformBufferStd140(desc, scratch);
    
    glNamedBufferData(uniformBuffer.buffer, buffer.len, buffer.ptr, GL_DYNAMIC_DRAW);
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
    GLint glslLen = (GLint)glsl.len;
    glShaderSource(shader, 1, &glsl.ptr, &glslLen);
    glCompileShader(shader);
    
    GLint status = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if(!status)
    {
        char info[512];
        glGetShaderInfoLog(shader, 512, NULL, info);
        Log("%s", info);
    }
    
    return {.kind = kind, .handle = shader};
}

R_Pipeline R_CreatePipeline(Slice<R_Shader> shaders)
{
    R_Pipeline res = {0};
    res.shaders = ArenaPushSlice(&sceneArena, shaders);
    res.handle = glCreateProgram();
    
    for(int i = 0; i < shaders.len; ++i)
        glAttachShader(res.handle, shaders[i].handle);
    
    glLinkProgram(res.handle);
    
    GLint status = 0;
    glGetProgramiv(res.handle, GL_LINK_STATUS, &status);
    if(!status)
    {
        char info[512];
        glGetProgramInfoLog(res.handle, 512, NULL, info);
        Log("%s", info);
    }
    
    return res;
}

void R_SetViewport(int width, int height)
{
    glViewport(0, 0, width, height);
}

void R_SetPipeline(R_Pipeline pipeline)
{
    glUseProgram(pipeline.handle);
    renderer.boundPipeline = pipeline;
}

void R_SetUniforms(u32 binding, Slice<R_UniformValue> desc)
{
    // Check the uniforms types
    
    // Construct the buffer to send to the GPU
    ScratchArena scratch;
    Slice<uchar> buffer = MakeUniformBufferStd140(desc, scratch);
    
    // Send the buffer to the GPU
    
}

void R_SetUniformBuffer(u32 index, Slice<R_UniformValue> desc)
{
    
}

void R_ClearFrame(Vec4 color)
{
    glClearColor(color.x, color.y, color.z, color.w);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

void R_EnableDepthTest(bool enable)
{
    if(enable)
        glEnable(GL_DEPTH_TEST);
    else
        glDisable(GL_DEPTH_TEST);
}

void R_EnableCullFace(bool enable)
{
    if(enable)
        glEnable(GL_CULL_FACE);
    else
        glDisable(GL_CULL_FACE);
}

void R_EnableAlphaBlending(bool enable)
{
    if(enable)
    {
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_BLEND);
    }
    else
        glDisable(GL_BLEND);
}

void R_Cleanup()
{
}

void R_ImDrawArrow(Vec3 ori, Vec3 dst, float baseRadius, float coneLength, float coneRadius, Vec4 color);
void R_ImDrawCone(Vec3 baseCenter, Vec3 dir, float length, float radius, Vec4 color);

void R_ImDrawCylinder(Vec3 center, float radius, float height, Vec4 color)
{
    
}

void R_ImDrawQuad(Vec3 v1, Vec3 v2, Vec3 v3, Vec3 v4, Vec4 color)
{
    TODO;
}

void R_RenderDearImgui()
{
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void R_ShutdownDearImgui()
{
    ImGui_ImplOpenGL3_Shutdown();
}

void Opengl_ImGuiShowTexture(GLuint texture)
{
    glBindTexture(GL_TEXTURE_2D, texture);
    
    int width, height;
    int miplevel = 0;
    glGetTexLevelParameteriv(GL_TEXTURE_2D, miplevel, GL_TEXTURE_WIDTH, &width);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, miplevel, GL_TEXTURE_HEIGHT, &height);
    
    ImVec2 avail = ImGui::GetContentRegionAvail();
    ImVec2 size = ImVec2(avail.x, avail.x * (float)height / (float)width);
    
    ImGui::Image((ImTextureID)(u64)texture, size, ImVec2(0, 0), ImVec2(1, 1));
    glBindTexture(GL_TEXTURE_2D, 0);
}
