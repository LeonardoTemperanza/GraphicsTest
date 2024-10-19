
// NOTE: This backend's development is on pause, so there are some missing
// functionalities. I'm currently focusing on the DX11 backend for many reasons.

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
        case R_TexR8:    return GL_R8;
        case R_TexRG8:   return GL_RG8;
        case R_TexRGB8:  return GL_RGB8;
        case R_TexRGBA8: return GL_RGBA8;
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
        case R_TexR8:    return GL_RED;
        case R_TexRG8:   return GL_RG;
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
        case R_TexR8:    return GL_UNSIGNED_BYTE;
        case R_TexRG8:   return GL_UNSIGNED_BYTE;
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

R_Shader Opengl_CompileShader(ShaderKind kind, String glsl)
{
    GLuint shader = glCreateShader(Opengl_ShaderKind(kind));
    GLint glslLen = (GLint)glsl.len;
    glShaderSource(shader, 1, &glsl.ptr, &glslLen);
    glCompileShader(shader);
    
    GLint status = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if(!status)
    {
        char info[1024];
        glGetShaderInfoLog(shader, sizeof(info), NULL, info);
        Log("%s", info);
    }
    
    return {.kind = kind, .handle = shader};
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
    
    framebuffer.width  = max(1, width);
    framebuffer.height = max(1, height);
    
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

// NOTE: These have to be defined correctly in the shader too!
// there should be a "cpp_hlsl_bridge" where i can put in stuff like this
// that is shared between the shader code and the cpp code
#define Opengl_PerSceneBinding 0
#define Opengl_PerFrameBinding 1
#define Opengl_PerObjBinding   2
#define Opengl_GlobalsBinding  3

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#include <windows.h>
// From: https://gist.github.com/mmozeiko/ed2ad27f75edf9c26053ce332a1f6647
void Opengl_Win32GetWGLFunctions()
{
    // To get WGL functions, we first need a valid
    // OpenGL context. For that, we need to create
    // a dummy window for a dummy OpenGL context
    HWND dummy = CreateWindowEx(0, "STATIC", "Dummy", WS_OVERLAPPED,
                                CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                nullptr, nullptr, nullptr, nullptr);
    
    assert(dummy && "Failed to create dummy window");
    
    HDC dc = GetDC(dummy);
    assert(dc && "Failed to get device context for dummy window");
    
    PIXELFORMATDESCRIPTOR desc = {0};
    desc.nSize      = sizeof(desc);
    desc.nVersion   = 1;
    desc.dwFlags    = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    desc.iPixelType = PFD_TYPE_RGBA;
    desc.cColorBits = 24;
    
    // NOTE: Whyyyyyy does this take ~1sec on my machine???
    // Seems to be a widespread driver problem that can't be fixed
    int format = ChoosePixelFormat(dc, &desc);
    if(!format)
        OS_FatalError("Cannot choose OpenGL pixel format for dummy window.");
    
    int success = DescribePixelFormat(dc, format, sizeof(desc), &desc);
    assert(success && "Failed to describe OpenGL pixel format");
    
    // Reason to create dummy window is that SetPixelFormat can be called only
    // once for this window
    if(!SetPixelFormat(dc, format, &desc))
        OS_FatalError("Cannot set OpenGL pixel format for dummy window.");
    
    HGLRC rc = wglCreateContext(dc);
    assert(rc && "Failed to create OpenGL context for dummy window.");
    
    success = wglMakeCurrent(dc, rc);
    assert(success && "Failed to make current OpenGL context for dummy window.");
    
    // https://www.khronos.org/registry/OpenGL/extensions/ARB/WGL_ARB_extensions_string.txt
    PFNWGLGETEXTENSIONSSTRINGARBPROC wglGetExtensionsStringARB = 
    (PFNWGLGETEXTENSIONSSTRINGARBPROC)wglGetProcAddress("wglGetExtensionsStringARB");
    
    if(!wglGetExtensionsStringARB)
        OS_FatalError("OpenGL does not support WGL_ARB_extensions_string extension!");
    
    const char* ext = wglGetExtensionsStringARB(dc);
    assert(ext && "Failed to get OpenGL WGL extension string");
    
    // Get the only three extensions we need to create the context
    const char* start = ext;
    while(true)
    {
        while(*ext != '\0' && *ext != ' ') ++ext;
        
        size_t length = ext - start;
        if(strncmp("WGL_ARB_pixel_format", start, length) == 0)
        {
            // https://www.khronos.org/registry/OpenGL/extensions/ARB/WGL_ARB_pixel_format.txt
            wgl.wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");
        }
        else if(strncmp("WGL_ARB_create_context", start, length) == 0)
        {
            // https://www.khronos.org/registry/OpenGL/extensions/ARB/WGL_ARB_create_context.txt
            wgl.wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");
        }
        else if(strncmp("WGL_EXT_swap_control", start, length) == 0)
        {
            // https://www.khronos.org/registry/OpenGL/extensions/EXT/WGL_EXT_swap_control.txt
            wgl.wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
        }
        
        if(*ext == '\0') break;
        
        ++ext;
        start = ext;
    }
    
    if(!wgl.wglChoosePixelFormatARB ||
       !wgl.wglCreateContextAttribsARB ||
       !wgl.wglSwapIntervalEXT)
    {
        OS_FatalError("OpenGL does not support required WGL extensions for modern context.");
    }
    
    wglMakeCurrent(nullptr, nullptr);
    wglDeleteContext(rc);
    ReleaseDC(dummy, dc);
    DestroyWindow(dummy);
}

static void APIENTRY OpenglDebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity,
                                         GLsizei length, const GLchar* message, const void* user)
{
    if(type == GL_DEBUG_TYPE_ERROR || type == GL_DEBUG_TYPE_PERFORMANCE)
    {
        Log("%s", message);
        DebugMessage(message);
        DebugMessage("\n");
        
        if(type == GL_DEBUG_TYPE_ERROR)
        {
            if(B_IsDebuggerPresent())
            {
                //assert(!"OpenGL error - check the callstack in debugger");
            }
            //OS_FatalError("OpenGL API usage error! Use debugger to examine call stack!");
        }
    }
}

#endif

void Opengl_InitContext()
{
    // Sadly this is platform dependent
#ifdef _WIN32
    Opengl_Win32GetWGLFunctions();
    
    // Set pixel format for OpenGL context
    {
        int attrib[] =
        {
            WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
            WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
            WGL_DOUBLE_BUFFER_ARB,  GL_TRUE,
            WGL_PIXEL_TYPE_ARB,     WGL_TYPE_RGBA_ARB,
            WGL_COLOR_BITS_ARB,     24,
            WGL_DEPTH_BITS_ARB,     24,
            WGL_STENCIL_BITS_ARB,   8,
            
            // Uncomment for sRGB framebuffer, from WGL_ARB_framebuffer_sRGB extension
            // https://www.khronos.org/registry/OpenGL/extensions/ARB/ARB_framebuffer_sRGB.txt
            //WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB, GL_TRUE,
            
            // uncomment for multisampeld framebuffer, from WGL_ARB_multisample extension
            // https://www.khronos.org/registry/OpenGL/extensions/ARB/ARB_multisample.txt
            WGL_SAMPLE_BUFFERS_ARB, 1,
            WGL_SAMPLES_ARB,        4, // 4x MSAA
            
            0
        };
        
        int format;
        UINT formats;
        int success = wgl.wglChoosePixelFormatARB(win32.windowDC, attrib, nullptr, 1, &format, &formats);
        if(!success || formats == 0)
            OS_FatalError("OpenGL does not support required pixel format.");
        
        PIXELFORMATDESCRIPTOR desc = { .nSize = sizeof(desc) };
        success = DescribePixelFormat(win32.windowDC, format, sizeof(desc), &desc);
        assert(success && "Failed to describe pixel format.");
        
        if(!SetPixelFormat(win32.windowDC, format, &desc))
            OS_FatalError("Cannot set OpenGL selected pixel format.");
    }
    
    // Create opengl context
    {
        int attrib[] =
        {
            WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
            WGL_CONTEXT_MINOR_VERSION_ARB, 6,
            WGL_CONTEXT_PROFILE_MASK_ARB,  WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
#ifndef NDEBUG
            WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB,
#endif
            0
        };
        
        wgl.glContext = wgl.wglCreateContextAttribsARB(win32.windowDC, nullptr, attrib);
        if(!wgl.glContext)
            OS_FatalError("Cannot create modern OpenGL context. OpenGL 4.6 might not be supported on this architecture.");
        
        bool success = wglMakeCurrent(win32.windowDC, wgl.glContext);
        assert(success && "Failed to make current OpenGL context");
        
        wgl.wglSwapIntervalEXT(1);
    }
    
    // Load opengl functions
    if(!gladLoadGLLoader((GLADloadproc)Win32_GetOpenGLProc))
        OS_FatalError("Failed to load OpenGL.");
    
#ifndef NDEBUG
    // Enable debug callback
    glDebugMessageCallback(&OpenglDebugCallback, NULL);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
#endif
    
#else  // #ifdef _WIN32
#error "Opengl context initialization is not implemented for this platform."
#endif
}

void R_Init()
{
    Renderer* r = &renderer;
    memset(r, 0, sizeof(Renderer));
    
    Opengl_InitContext();
    
    // Uniform buffers
    
    // Per frame uniform buffer
    {
        GLuint buffer;
        GLuint binding = Opengl_PerFrameBinding;
        glCreateBuffers(1, &buffer);
        glBindBufferBase(GL_UNIFORM_BUFFER, binding, buffer);
        
        r->perFrameBuffer = buffer;
    }
    
    // Per Obj uniform buffer
    {
        GLuint buffer;
        GLuint binding = Opengl_PerObjBinding;
        glCreateBuffers(1, &buffer);
        glBindBufferBase(GL_UNIFORM_BUFFER, binding, buffer);
        
        r->perObjBuffer = buffer;
    }
    
    // "_Globals" uniform buffer
    {
        GLuint buffer;
        GLuint binding = Opengl_GlobalsBinding;
        glCreateBuffers(1, &buffer);
        glBindBufferBase(GL_UNIFORM_BUFFER, binding, buffer);
        
        r->globalsBuffer = buffer;
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
    
    // Non-fullscreen quad
    {
        glCreateVertexArrays(1, &r->quadVao);
        glCreateBuffers(1, &r->quadVbo);
        
        glVertexArrayVertexBuffer(r->quadVao, 0, r->quadVbo, 0, 3 * sizeof(float));
        glEnableVertexArrayAttrib(r->quadVao, 0);
        glVertexArrayAttribBinding(r->quadVao, 0, 0);
        glVertexArrayAttribFormat(r->quadVao, 0, 3, GL_FLOAT, GL_FALSE, 0);
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
    
    // Unit cone
    {
        BasicMesh cone = GenerateUnitCone();
        r->unitConeCount = (u32)cone.indices.len;
        
        glCreateVertexArrays(1, &r->unitCone);
        GLuint vbo;
        GLuint ebo;
        glCreateBuffers(1, &vbo);
        glCreateBuffers(1, &ebo);
        glNamedBufferData(vbo, cone.verts.len * sizeof(Vec3), cone.verts.ptr, GL_STATIC_DRAW);
        glNamedBufferData(ebo, cone.indices.len * sizeof(cone.indices[0]), cone.indices.ptr, GL_STATIC_DRAW);
        
        glVertexArrayVertexBuffer(r->unitCone, 0, vbo, 0, 3 * sizeof(float));
        glVertexArrayElementBuffer(r->unitCone, ebo);
        
        glEnableVertexArrayAttrib(r->unitCone, 0);
        glVertexArrayAttribBinding(r->unitCone, 0, 0);
        glVertexArrayAttribFormat(r->unitCone, 0, 3, GL_FLOAT, GL_FALSE, 0);
    }
}

void R_DrawMesh(R_Mesh mesh)
{
    if(mesh.numIndices <= 0) return;
    
    Renderer* r = &renderer;
    
    glBindVertexArray(mesh.vao);
    glDrawElements(GL_TRIANGLES, mesh.numIndices, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

Mat4 R_ConvertClipSpace(Mat4 mat)
{
    // I calculate the matrix for [-1, 1] in all axes with x pointing right,
    // y pointing up, z pointing into the screen. Opengl is essentially the same
    // except the z axis points out of the screen, so it needs to be flipped here.
    mat.m13 *= -1;
    mat.m23 *= -1;
    mat.m33 *= -1;
    mat.m43 = 1;
    return mat;
}

R_Mesh R_UploadMesh(Slice<Vertex> verts, Slice<s32> indices)
{
    GLuint vao;
    glCreateVertexArrays(1, &vao);
    GLuint vbo, ebo;
    constexpr int numBufs = 2;
    GLuint bufferIds[numBufs];
    glCreateBuffers(2, bufferIds);
    int curIdx = 0;
    vbo = bufferIds[curIdx++];
    ebo = bufferIds[curIdx++];
    assert(curIdx == numBufs);
    
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
    
    R_Mesh res = {0};
    res.vao = vao;
    res.numIndices = (u32)indices.len;
    return res;
}

R_Texture R_UploadTexture(String blob, u32 width, u32 height, u8 numChannels)
{
    assert(numChannels >= 1 && numChannels <= 4);
    
    R_Texture res = {0};
    glGenTextures(1, &res.handle);
    
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
            case 1:  format = GL_RED;  res.format = R_TexR8;    break;
            case 2:  format = GL_RG;   res.format = R_TexRG8;   break;
            case 3:  format = GL_RGB;  res.format = R_TexRGB8;  break;
            case 4:  format = GL_RGBA; res.format = R_TexRGBA8; break;
        }
        
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, blob.ptr);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    
    return res;
}

R_Texture R_UploadCubemap(String top, String bottom, String left, String right, String front, String back, u32 width, u32 height, u8 numChannels)
{
    R_Texture res = {};
    res.kind = R_TexCubemap;
    
    glGenTextures(1, &res.handle);
    glBindTexture(GL_TEXTURE_CUBE_MAP, res.handle);
    
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    
    // NOTE: The order is chosen so that we can loop through the maps
    // in the order in which the enum is defined in opengl
    String cubemapTextureBlobs[6] = { right, left, top, bottom, front, back };
    for(int i = 0; i < 6; ++i)
    {
        GLint format = GL_RGB;
        switch(numChannels)
        {
            default: TODO; break;
            case 1:  format = GL_RED;  res.format = R_TexR8;    break;
            case 2:  format = GL_RG;   res.format = R_TexRG8;   break;
            case 3:  format = GL_RGB;  res.format = R_TexRGB8;  break;
            case 4:  format = GL_RGBA; res.format = R_TexRGBA8; break;
        }
        
        void* data = (void*)cubemapTextureBlobs[i].ptr;
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                     0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    }
    
    return res;
}

R_Shader R_CreateDefaultShader(ShaderKind kind)
{
    String vertShader = StrLit(R""""(
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       #version 460 core
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            void main()
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 {
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      gl_Position = vec4(0.0f, 0.0f, -2.0f, 0.0f);
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           }
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                )"""");
    
    String pixelShader = StrLit(R""""(
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            #version 460 core
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                out vec4 fragColor;
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    void main()
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        {
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            fragColor = vec4(1.0f, 0.0f, 1.0f, 1.0f);
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                }
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    )"""");
    
    String computeShader = StrLit(R""""(
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            #version 460 core
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                void main() { }
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  )"""");
    
    switch(kind)
    {
        case ShaderKind_None:    return Opengl_CompileShader(ShaderKind_Vertex,  vertShader);
        case ShaderKind_Count:   return Opengl_CompileShader(ShaderKind_Vertex,  vertShader);
        case ShaderKind_Vertex:  return Opengl_CompileShader(ShaderKind_Vertex,  vertShader);
        case ShaderKind_Pixel:   return Opengl_CompileShader(ShaderKind_Pixel,   pixelShader);
        case ShaderKind_Compute: return Opengl_CompileShader(ShaderKind_Compute, computeShader);
    }
    
    return R_CompileShader(ShaderKind_Vertex,  StrLit(""), StrLit(""), vertShader);
}

R_Mesh R_CreateDefaultMesh()
{
    GLuint vao;
    glCreateVertexArrays(1, &vao);
    GLuint vbo, ebo;
    constexpr int numBufs = 2;
    GLuint bufferIds[numBufs];
    glCreateBuffers(2, bufferIds);
    int curIdx = 0;
    vbo = bufferIds[curIdx++];
    ebo = bufferIds[curIdx++];
    assert(curIdx == numBufs);
    
    glNamedBufferData(vbo, 0, nullptr, GL_STATIC_DRAW);
    glNamedBufferData(ebo, 0, nullptr, GL_STATIC_DRAW);
    glEnableVertexArrayAttrib(vao, 0);  // Position
    glEnableVertexArrayAttrib(vao, 1);  // Normal
    glEnableVertexArrayAttrib(vao, 2);  // Texture coords
    glEnableVertexArrayAttrib(vao, 3);  // Tangents
    
    glVertexArrayVertexBuffer(vao, 0, vbo, 0, 0);
    glVertexArrayElementBuffer(vao, ebo);
    R_Mesh res = {};
    res.vao = vao;
    res.numIndices = 0;
    return res;
}

R_Shader R_CreateShader(ShaderKind kind, ShaderInput input)
{
    return Opengl_CompileShader(kind, input.glsl);
}

R_Pipeline R_CreatePipeline(Slice<R_Shader> shaders)
{
    R_Pipeline res = {0};
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
        Log("Shader pipeline creation error: %s", info);
        
        // Redo shader program with default shaders
        glDeleteProgram(res.handle);
        
        R_Shader defaultShaders[] =
        {
            R_MakeDefaultShader(ShaderKind_Vertex),
            R_MakeDefaultShader(ShaderKind_Pixel)
        };
        
        res.handle = glCreateProgram();
        for(int i = 0; i < ArrayCount(defaultShaders); ++i)
            glAttachShader(res.handle, defaultShaders[i].handle);
        glLinkProgram(res.handle);
        
        GLint status = 0;
        glGetProgramiv(res.handle, GL_LINK_STATUS, &status);
        assert(status);  // Linking default shaders shouldn't fail
        return res;
    }
    
    // Successfully created the pipeline
    
    res.globalsUniformBlockIndex = glGetUniformBlockIndex(res.handle, "type_Globals");
    if(res.globalsUniformBlockIndex != -1)
    {
        glUniformBlockBinding(res.handle, res.globalsUniformBlockIndex, Opengl_GlobalsBinding);
        res.hasGlobals = true;
    }
    
    // @tmp
    //for(int i = 0; i < 2; ++i)
    //glProgramUniform1i(res.handle, i, i);
    
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
    Renderer* r = &renderer;
    
    if(desc.len == 0) return;
    
    if(!renderer.boundPipeline.hasGlobals)
    {
        //Log("Trying to set uniforms, but the current shader doesn't have any");
        return;
    }
    
    // TODO: Check the uniforms types (not in release)
    {
        
    }
    
    // NOTE: Assuming the uniform buffers are padded with
    // std140!! Changing the standard used (by modifying the
    // shader importer) will make the renderer suddenly incorrect
    ScratchArena scratch;
    Slice<uchar> buffer = MakeUniformBufferStd140(desc, scratch);
    
    glNamedBufferData(r->globalsBuffer, buffer.len, buffer.ptr, GL_DYNAMIC_DRAW);
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

Vec4 R_ReadPixelFromFramebuffer(int x, int y)
{
    R_TextureFormat format = renderer.boundFramebuffer.colorFormat;
    assert(!IsTextureFormatInteger(format));
    
    // TODO: Test this for multiple types of textures
    
    Vec4 res;
    glReadPixels(x, y, 1, 1, Opengl_PixelDataFormat(format), GL_FLOAT, &res);
    
    return res;
}

void R_SetTexture(R_Texture texture, u32 slot)
{
    glActiveTexture(GL_TEXTURE0 + slot);
    switch(texture.kind)
    {
        case R_Tex2D:      glBindTexture(GL_TEXTURE_2D,       texture.handle); break;
        case R_TexCubemap: glBindTexture(GL_TEXTURE_CUBE_MAP, texture.handle); break;
    }
    
}

void R_SetPerSceneData(R_PerSceneData perScene)
{
    TODO;
}

void R_SetPerFrameData(Mat4 world2View, Mat4 view2Proj, Vec3 viewPos)
{
    Renderer* r = &renderer;
    
    struct alignas(16) PerFrameDataStd140
    {
        Mat4 world2View;
        Mat4 view2Proj;
        Vec3 viewPos;
        float padding;
    } perFrameStd140;
    
    // Fill in the struct with the correct layout
    perFrameStd140.world2View = world2View;
    perFrameStd140.view2Proj  = view2Proj;
    perFrameStd140.viewPos    = viewPos;
    perFrameStd140.padding    = 0.0f;
    
    glNamedBufferData(r->perFrameBuffer, sizeof(perFrameStd140), &perFrameStd140, GL_DYNAMIC_DRAW);
}

void R_SetPerObjData(Mat4 model2World, Mat3 normalMat)
{
    Renderer* r = &renderer;
    
    struct alignas(16) PerObjDataStd140
    {
        Mat4 model2World;
        float normalMat[3][4];
    } perObjStd140;
    
    // Fill in the struct with the correct layout
    perObjStd140.model2World = model2World;
    
    // @speed
    for(int i = 0; i < 3; ++i)
    {
        for(int j = 0; j < 3; ++j)
        {
            perObjStd140.normalMat[i][j] = normalMat.m[i][j];
        }
    }
    
    glNamedBufferData(r->perObjBuffer, sizeof(perObjStd140), &perObjStd140, GL_DYNAMIC_DRAW);
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

void R_ClearDepth()
{
    glClear(GL_DEPTH_BUFFER_BIT);
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

void R_DrawCone(Vec3 baseCenter, Vec3 dir, float radius, float length)
{
    Quat rot = FromToRotation(Vec3::up, dir);
    Vec3 scale = {radius, length, radius};
    Mat4 model2World = Mat4FromPosRotScale(baseCenter, rot, scale);
    R_SetPerObjData(model2World, Mat3::identity);
    
    glBindVertexArray(renderer.unitCone);
    glDrawElements(GL_TRIANGLES, (GLsizei)renderer.unitConeCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void R_DrawCylinder(Vec3 center, Vec3 dir, float radius, float height)
{
    Quat rot = FromToRotation(Vec3::up, dir);
    Vec3 scale = {radius, height, radius};
    Mat4 model2World = Mat4FromPosRotScale(center, rot, {radius, height, radius});
    R_SetPerObjData(model2World, Mat3::identity);
    
    glBindVertexArray(renderer.unitCylinder);
    glDrawElements(GL_TRIANGLES, (GLsizei)renderer.unitCylinderCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void R_DrawQuad(Vec3 v1, Vec3 v2, Vec3 v3, Vec3 v4)
{
    Renderer* r = &renderer;
    
    Vec3 verts[] =
    {
        v1, v2, v3,
        v1, v3, v4
    };
    
    glNamedBufferData(r->quadVbo, sizeof(verts), verts, GL_DYNAMIC_DRAW);
    
    glBindVertexArray(r->quadVao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void R_DrawFullscreenQuad()
{
    glBindVertexArray(renderer.fullscreenQuad);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void R_SubmitFrame()
{
    OS_OpenglSwapBuffers();
}

void R_DearImguiInit()
{
    ImGui_ImplOpenGL3_Init();
}

void R_DearImguiBeginFrame()
{
    ImGui_ImplOpenGL3_NewFrame();
}

void R_DearImguiRender()
{
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void R_DearImguiShutdown()
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
