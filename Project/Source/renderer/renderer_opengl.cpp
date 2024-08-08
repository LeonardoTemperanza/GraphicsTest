
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

GLuint Opengl_ShaderKind(ShaderKind kind)
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

GLuint Opengl_InternalFormat(R_TextureFormat format)
{
    switch(format)
    {
        case R_TexNone:  return GL_RGBA8;
        case R_TexRGB8:  return GL_RGB;
        case R_TexRGBA8: return GL_RGBA;
        case R_TexR8I:   return GL_R8I;
        case R_TexR8UI:  return GL_R8UI;
        case R_TexR32I:  return GL_R32I;
    }
    
    return GL_RGBA8;
}

GLuint Opengl_PixelDataFormat(R_TextureFormat format)
{
    switch(format)
    {
        case R_TexNone:  return GL_RGBA;
        case R_TexRGB8:  return GL_RGB;
        case R_TexRGBA8: return GL_RGBA;
        case R_TexR8I:   return GL_RED_INTEGER;
        case R_TexR8UI:  return GL_RED_INTEGER;
        case R_TexR32I:  return GL_RED_INTEGER;
    }
    
    return GL_RGBA;
}

GLuint Opengl_PixelDataType(R_TextureFormat format)
{
    switch(format)
    {
        case R_TexNone:  return GL_FLOAT;
        case R_TexRGB8:  return GL_UNSIGNED_BYTE;
        case R_TexRGBA8: return GL_UNSIGNED_BYTE;
        case R_TexR8I:   return GL_BYTE;
        case R_TexR8UI:  return GL_UNSIGNED_BYTE;
        case R_TexR32I:  return GL_INT;
    }
    
    return GL_FLOAT;
}

R_Framebuffer R_DefaultFramebuffer()
{
    return {0};
}

void Opengl_SetupColorTextureForFramebuffer(int width, int height, R_TextureFormat format, GLuint texture)
{
    auto internalFormat  = Opengl_InternalFormat(format);
    auto pixelDataFormat = Opengl_PixelDataFormat(format);
    auto pixelDataType   = Opengl_PixelDataType(format);
    
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, pixelDataFormat, pixelDataType, nullptr);
    
    if(IsTextureFormatInteger(format))
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }
    else
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
}

void Opengl_SetupDepthTextureForFramebuffer(int width, int height, GLuint texture)
{
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);
}

void Opengl_SetupStencilTextureForFramebuffer(int width, int height, GLuint texture)
{
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_STENCIL_INDEX8, width, height, 0, GL_STENCIL_INDEX8, GL_UNSIGNED_INT, nullptr);
}

R_Framebuffer R_CreateFramebuffer(int width, int height, bool color, R_TextureFormat colorFormat, bool depth, bool stencil)
{
    width  = max(width, 1);
    height = max(height, 1);
    
    R_Framebuffer res = {0};
    res.width  = width;
    res.height = height;
    res.color = color;
    res.depth = depth;
    res.stencil = stencil;
    res.colorFormat = colorFormat;
    
    glCreateFramebuffers(1, &res.handle);
    
    int numTextures = color + depth + stencil;
    assert(numTextures <= gl_FramebufferMaxTextures && "Trying to use more textures than the max allowed number");
    glGenTextures(numTextures, res.textures);
    
    int curTexture = 0;
    if(color)
    {
        Opengl_SetupColorTextureForFramebuffer(width, height, colorFormat, res.textures[curTexture]);
        glNamedFramebufferTexture(res.handle, GL_COLOR_ATTACHMENT0, res.textures[curTexture], 0);
        ++curTexture;
    }
    else
        glNamedFramebufferDrawBuffer(res.handle, GL_NONE);
    
    if(depth)
    {
        Opengl_SetupDepthTextureForFramebuffer(width, height, res.textures[curTexture]);
        glNamedFramebufferTexture(res.handle, GL_DEPTH_ATTACHMENT, res.textures[curTexture], 0);
        ++curTexture;
    }
    
    if(stencil)
    {
        Opengl_SetupStencilTextureForFramebuffer(width, height, res.textures[curTexture]);
        glNamedFramebufferTexture(res.handle, GL_STENCIL_ATTACHMENT, res.textures[curTexture], 0);
        ++curTexture;
    }
    
    GLenum ok = glCheckNamedFramebufferStatus(res.handle, GL_FRAMEBUFFER);
    assert(ok == GL_FRAMEBUFFER_COMPLETE);
    return res;
}

void R_ResizeFramebuffer(R_Framebuffer framebuffer, int width, int height)
{
    if(framebuffer.width == width && framebuffer.height == height)
        return;
    
    framebuffer.width  = width;
    framebuffer.height = height;
    
    int curTexture = 0;
    if(framebuffer.color)
    {
        Opengl_SetupColorTextureForFramebuffer(width, height, framebuffer.colorFormat, framebuffer.textures[curTexture]);
        ++curTexture;
    }
    
    if(framebuffer.depth)
    {
        Opengl_SetupDepthTextureForFramebuffer(width, height, framebuffer.textures[curTexture]);
        ++curTexture;
    }
    
    if(framebuffer.stencil)
    {
        Opengl_SetupStencilTextureForFramebuffer(width, height, framebuffer.textures[curTexture]);
        ++curTexture;
    }
}

// TODO: Should probably figure out a better API for this,
// so that we don't have to do this. But for now it's fine,
// and it lets us proceed with the game and find out what
// the API should even look like
#define Opengl_ReservedUniformBufferBinding 10

void R_Init()
{
    Renderer* r = &renderer;
    memset(r, 0, sizeof(Renderer));
    
    // TODO: This should actually initialize the opengl
    // context. Right now it resides in the platform layer
    // but i think it would be best if we handled it here,
    // using some #ifdefs with platform specific code in there
    
    // "_Globals" uniform buffer
    {
        GLuint buffer;
        GLuint binding = Opengl_ReservedUniformBufferBinding;
        glCreateBuffers(1, &buffer);
        glBindBufferBase(GL_UNIFORM_BUFFER, binding, buffer);
        
        r->globalsUniformBuffer.buffer  = buffer;
        r->globalsUniformBuffer.binding = binding;
    }
    
    // Create objects used for simple rendering
    // Fullscreen quad
    {
        glCreateVertexArrays(1, &r->fullscreenQuad);
        GLuint vbo;
        glCreateBuffers(1, &vbo);
        
        float fullscreenVerts[] =
        {
            -1, -1, 0,
            1, -1, 0,
            -1, 1, 0,
            -1, 1, 0,
            1, -1, 0,
            1, 1, 0,
        };
        
        glNamedBufferData(vbo, sizeof(fullscreenVerts), fullscreenVerts, GL_STATIC_DRAW);
        glVertexArrayVertexBuffer(r->fullscreenQuad, 0, vbo, 0, 3 * sizeof(float));
        
        glEnableVertexArrayAttrib(r->fullscreenQuad, 0);
        glVertexArrayAttribBinding(r->fullscreenQuad, 0, 0);
        glVertexArrayAttribFormat(r->fullscreenQuad, 0, 3, GL_FLOAT, GL_FALSE, 0);
    }
    
    // Unit cylinder
    {
        BasicMesh cylinder = GenerateUnitCylinder();
        r->unitCylinderCount = (u32)cylinder.indices.len;
        
        glCreateVertexArrays(1, &r->unitCylinder);
        GLuint vbo;
        GLuint ebo;
        glCreateBuffers(1, &vbo);
        glCreateBuffers(1, &ebo);
        glNamedBufferData(vbo, cylinder.verts.len * sizeof(Vec3), cylinder.verts.ptr, GL_STATIC_DRAW);
        glNamedBufferData(ebo, cylinder.indices.len * sizeof(cylinder.indices[0]), cylinder.indices.ptr, GL_STATIC_DRAW);
        
        glVertexArrayVertexBuffer(r->unitCylinder, 0, vbo, 0, 3 * sizeof(float));
        glVertexArrayElementBuffer(r->unitCylinder, ebo);
        
        glEnableVertexArrayAttrib(r->unitCylinder, 0);
        glVertexArrayAttribBinding(r->unitCylinder, 0, 0);
        glVertexArrayAttribFormat(r->unitCylinder, 0, 3, GL_FLOAT, GL_FALSE, 0);
    }
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
    assert(binding != Opengl_ReservedUniformBufferBinding && "Trying to use a binding that's taken by the _Globals Uniform Buffer");
    
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
    
    R_Texture res = {0};
    glGenTextures(1, &res.handle);
    
    // TODO: Use bindless API?
    glBindTexture(GL_TEXTURE_2D, res.handle);
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        GLint format = GL_RGB;
        switch(numChannels)
        {
            default: TODO; break;
            case 2:  format = GL_RG;   TODO; break; // TODO
            case 3:  format = GL_RGB;  res.format = R_TexRGB8;  break;
            case 4:  format = GL_RGBA; res.format = R_TexRGBA8; break;
        }
        
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, blob.ptr);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    
    return res;
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
    
    int count;
    glGetProgramiv(res.handle, GL_ACTIVE_UNIFORMS, &count);
    Log("Active Uniforms: %d\n", count);
    
    res.globalsUniformBlockIndex = glGetUniformBlockIndex(res.handle, "type_Globals");
    if(res.globalsUniformBlockIndex != -1)
        res.hasGlobals = true;
    
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

void R_SetUniforms(Slice<R_UniformValue> desc)
{
    if(desc.len == 0) return;
    
    if(!renderer.boundPipeline.hasGlobals)
    {
        Log("Trying to set uniforms, but the current shader doesn't have any");
        return;
    }
    
    // TODO: Check the uniforms types (not in release)
    {
        
    }
    
    R_UploadUniformBuffer(renderer.globalsUniformBuffer, desc);
    
    // Bind this buffer to the shader
    // NOTE TODO This should probably not be here but in the
    // shader loading code
    GLuint globalsIndex = renderer.boundPipeline.globalsUniformBlockIndex;
    GLuint globalsBinding = renderer.globalsUniformBuffer.binding;
    glUniformBlockBinding(renderer.boundPipeline.handle, globalsIndex, globalsBinding);
}

void R_SetFramebuffer(R_Framebuffer framebuffer)
{
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.handle);
    renderer.boundFramebuffer = framebuffer;
}

R_Texture R_GetFramebufferColorTexture(R_Framebuffer framebuffer)
{
    assert(framebuffer.color);
    
    R_Texture res = {0};
    res.handle = framebuffer.textures[0];
    res.format = framebuffer.colorFormat;
    return res;
}

int R_ReadIntPixelFromFramebuffer(int x, int y)
{
    R_TextureFormat format = renderer.boundFramebuffer.colorFormat;
    assert(IsTextureFormatInteger(format));
    
    // TODO: Test this for multiple types of textures
    
    int res = 0;
    if(IsTextureFormatSigned(format))
        glReadPixels(x, y, 1, 1, GL_RED_INTEGER, GL_INT, &res);
    else
    {
        unsigned int unsignedVal = 0;
        glReadPixels(x, y, 1, 1, GL_RED_INTEGER, GL_UNSIGNED_INT, &unsignedVal);
        res = (int)unsignedVal;
    }
    
    return res;
}

int R_ReadIntPixelFromFramebufferAndGetItNextFrame(R_Texture texture, int x, int y)
{
    TODO;
    return 0;
}

Vec4 R_ReadPixelFromFramebuffer(int x, int y)
{
    R_TextureFormat format = renderer.boundFramebuffer.colorFormat;
    assert(!IsTextureFormatInteger(format));
    
    // TODO: Test this for multiple types of textures
    
    Vec4 res;
    glReadPixels(x, y, 1, 1, Opengl_PixelDataFormat(format), GL_FLOAT, &res);
    
    return res;
}

Vec4 R_ReadPixelFromTextureAndGetItNextFrame(R_Texture texture, int x, int y)
{
    TODO;
    return {0};
}

void R_SetTexture(R_Texture texture, u32 slot)
{
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, texture.handle);
}

void R_ClearFrame(Vec4 color)
{
    glClearColor(color.x, color.y, color.z, color.w);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

void R_ClearFrameInt(int r, int g, int b, int a)
{
    assert(IsTextureFormatInteger(renderer.boundFramebuffer.colorFormat));
    
    int values[] = {r, g, b, a};
    glClearBufferiv(GL_COLOR, 0, values);
    glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

void R_DepthTest(bool enable)
{
    if(enable)
        glEnable(GL_DEPTH_TEST);
    else
        glDisable(GL_DEPTH_TEST);
}

void R_CullFace(bool enable)
{
    if(enable)
        glEnable(GL_CULL_FACE);
    else
        glDisable(GL_CULL_FACE);
}

void R_AlphaBlending(bool enable)
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

void R_DrawArrow(Vec3 ori, Vec3 dst, float baseRadius, float coneLength, float coneRadius)
{
    TODO;
}

void R_DrawCone(Vec3 baseCenter, Vec3 dir, float length, float radius)
{
    TODO;
}

void R_DrawCylinder(Vec3 center, float radius, float height)
{
    glBindVertexArray(renderer.unitCylinder);
    glDrawElements(GL_TRIANGLES, (GLsizei)renderer.unitCylinderCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void R_DrawQuad(Vec3 v1, Vec3 v2, Vec3 v3, Vec3 v4)
{
    TODO;
}

void R_DrawFullscreenQuad()
{
    glBindVertexArray(renderer.fullscreenQuad);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void R_RenderDearImgui()
{
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void R_ShutdownDearImgui()
{
    ImGui_ImplOpenGL3_Shutdown();
}

// TODO: We want a function that lets us show a texture
// with DearImgui, regardless of the renderer, and of the
// texture format and size

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
