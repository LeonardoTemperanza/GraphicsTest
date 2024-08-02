
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
    
    // Initialize Vaos for common drawing operations
    glCreateVertexArrays(1, &r->fullScreenQuadVao);
    
    GLuint vbo;
    glCreateBuffers(1, &vbo);
    
    glNamedBufferData(vbo, sizeof(quadVerts), quadVerts, GL_STATIC_DRAW);
    
    // Position
    glEnableVertexArrayAttrib(r->fullScreenQuadVao, 0);
    glVertexArrayAttribBinding(r->fullScreenQuadVao, 0, 0);
    glVertexArrayAttribFormat(r->fullScreenQuadVao, 0, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayVertexBuffer(r->fullScreenQuadVao, 0, vbo, 0, sizeof(quadVerts[0]));
    
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
    r->basicProgram = glCreateProgram();
    r->mousePickingProgram = glCreateProgram();
    r->selectionProgram = glCreateProgram();
    r->outlineProgram = glCreateProgram();
    
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
    
    // Compile shaders for basic rendering utilities
    
    // Just draws something with a color
    const char* basicVertShader = R"(
#version 410 core

layout(location = 0) in vec3 pos;

uniform mat4 transform;

void main()
{
gl_Position = transform * vec4(pos, 1.0f);
}
)";
    
    {
        GLuint shader = glCreateShader(GL_VERTEX_SHADER);
        int size = (int)strlen(basicVertShader);
        glShaderSource(shader, 1, &basicVertShader, &size);
        glCompileShader(shader);
        
        GLint status = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
        if(!status)
        {
            char info[512];
            glGetShaderInfoLog(shader, 512, NULL, info);
            Log("Basic Vertex Shader: %s", info);
        }
        
        glAttachShader(r->basicProgram, shader);
        glAttachShader(r->outlineProgram, shader);
    }
    
    const char* basicFragShader = R"(
#version 410 core

layout(location = 0) in vec4 pos;
layout(location = 0) out vec4 fragColor;

uniform vec4 color;

void main()
{
fragColor = color;
}
)";
    
    {
        GLuint shader = glCreateShader(GL_FRAGMENT_SHADER);
        int size = (int)strlen(basicFragShader);
        glShaderSource(shader, 1, &basicFragShader, &size);
        glCompileShader(shader);
        
        GLint status = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
        if(!status)
        {
            char info[512];
            glGetShaderInfoLog(shader, 512, NULL, info);
            Log("Basic Fragment Shader: %s", info);
        }
        
        glAttachShader(r->basicProgram, shader);
    }
    
    glLinkProgram(r->basicProgram);
    
    r->colorUniform     = glGetUniformLocation(r->basicProgram, "color");
    r->transformUniform = glGetUniformLocation(r->basicProgram, "transform");
    
    // Mouse picking
    
    const char* mousePickingVertShader = R"(
#version 460 core

layout(binding = 0, std140) uniform type_PerFrame
{
    layout(row_major) mat4 world2View;
    layout(row_major) mat4 view2Proj;
    vec3 viewPos;
} PerFrame;

layout(binding = 1, std140) uniform type_PerObj
{
    layout(row_major) mat4 model2World;
} PerObj;

layout(location = 0) in vec3 in_var_POSITION;

mat4 spvWorkaroundRowMajor(mat4 wrap) { return wrap; }

void main()
{
    gl_Position = spvWorkaroundRowMajor(PerFrame.view2Proj) * (spvWorkaroundRowMajor(PerFrame.world2View) * (spvWorkaroundRowMajor(PerObj.model2World) * vec4(in_var_POSITION, 1.0)));
}
)";
    
    {
        GLuint shader = glCreateShader(GL_VERTEX_SHADER);
        int size = (int)strlen(mousePickingVertShader);
        glShaderSource(shader, 1, &mousePickingVertShader, &size);
        glCompileShader(shader);
        
        GLint status = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
        if(!status)
        {
            char info[512];
            glGetShaderInfoLog(shader, 512, NULL, info);
            Log("Mouse Picking Vertex Shader: %s", info);
        }
        
        glAttachShader(r->mousePickingProgram, shader);
        glAttachShader(r->selectionProgram, shader);
    }
    
    const char* mousePickingFragShader = R"(
#version 460 core

layout(location = 0) out ivec3 out_var_SV_TARGET;

uniform int id;

void main()
{
    out_var_SV_TARGET = ivec3(id);
}
)";
    
    {
        GLuint shader = glCreateShader(GL_FRAGMENT_SHADER);
        int size = (int)strlen(mousePickingFragShader);
        glShaderSource(shader, 1, &mousePickingFragShader, &size);
        glCompileShader(shader);
        
        GLint status = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
        if(!status)
        {
            char info[512];
            glGetShaderInfoLog(shader, 512, NULL, info);
            Log("Mouse Picking Fragment Shader: %s", info);
        }
        
        glAttachShader(r->mousePickingProgram, shader);
    }
    
    glLinkProgram(r->mousePickingProgram);
    GLint success;
    glGetProgramiv(r->mousePickingProgram, GL_LINK_STATUS, &success);
    if(!success)
    {
        char infoLog[512];
        glGetProgramInfoLog(r->mousePickingProgram, 512, NULL, infoLog);
        DebugMessage(infoLog);
    }
    
    r->mousePickingIdUniform = glGetUniformLocation(r->mousePickingProgram, "id");
    
    {
        int perFrameBindingPoint = 0;
        GLuint perFrameUniformIdx = glGetUniformBlockIndex(r->mousePickingProgram, "PerFrame");
        if(perFrameUniformIdx == -1) perFrameUniformIdx = 0;
        
        glBindBufferBase(GL_UNIFORM_BUFFER, perFrameBindingPoint, r->frameUbo);
        glUniformBlockBinding(r->mousePickingProgram, perFrameUniformIdx, perFrameBindingPoint);
        
        int perObjBindingPoint = 1;
        GLuint perObjUniformIdx = glGetUniformBlockIndex(r->mousePickingProgram, "PerObj");
        if(perObjUniformIdx == -1) perObjUniformIdx = 1;
        
        glBindBufferBase(GL_UNIFORM_BUFFER, perObjBindingPoint, r->objUbo);
        glUniformBlockBinding(r->mousePickingProgram, perObjUniformIdx, perObjBindingPoint);
    }
    
    const char* selectionFragShader = R"(
#version 460 core

layout(location = 0) out uvec3 out_var_SV_TARGET;

void main()
{
out_var_SV_TARGET = uvec3(1);
}
)";
    
    {
        GLuint shader = glCreateShader(GL_FRAGMENT_SHADER);
        int size = (int)strlen(selectionFragShader);
        glShaderSource(shader, 1, &selectionFragShader, &size);
        glCompileShader(shader);
        
        GLint status = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
        if(!status)
        {
            char info[512];
            glGetShaderInfoLog(shader, 512, NULL, info);
            Log("Mouse Picking Fragment Shader: %s", info);
        }
        
        glAttachShader(r->selectionProgram, shader);
    }
    
    glLinkProgram(r->selectionProgram);
    glGetProgramiv(r->selectionProgram, GL_LINK_STATUS, &success);
    if(!success)
    {
        char infoLog[512];
        glGetProgramInfoLog(r->selectionProgram, 512, NULL, infoLog);
        DebugMessage(infoLog);
    }
    
    {
        int perFrameBindingPoint = 0;
        GLuint perFrameUniformIdx = glGetUniformBlockIndex(r->selectionProgram, "PerFrame");
        if(perFrameUniformIdx == -1) perFrameUniformIdx = 0;
        
        glBindBufferBase(GL_UNIFORM_BUFFER, perFrameBindingPoint, r->frameUbo);
        glUniformBlockBinding(r->selectionProgram, perFrameUniformIdx, perFrameBindingPoint);
        
        int perObjBindingPoint = 1;
        GLuint perObjUniformIdx = glGetUniformBlockIndex(r->selectionProgram, "PerObj");
        if(perObjUniformIdx == -1) perObjUniformIdx = 1;
        
        glBindBufferBase(GL_UNIFORM_BUFFER, perObjBindingPoint, r->objUbo);
        glUniformBlockBinding(r->selectionProgram, perObjUniformIdx, perObjBindingPoint);
    }
    
    const char* outlineFragShader = R"(
#version 460 core

layout(location = 0) out vec4 out_var_SV_TARGET;

uniform vec4 color;
uniform usampler2D tex;

void main()
{
 uint foundSelected = 0;
 uint foundNonSelected = 0;
for(int y = -2; y <= 2; y++)
{
for(int x = -2; x <= 2; x++)
{
ivec2 sampleCoord = ivec2(int(floor(gl_FragCoord.x)) + x, int(floor(gl_FragCoord.y)) + y);
  uint sampled = texelFetch(tex, sampleCoord, 0).x;
if(sampled == 0) foundNonSelected++;
else             foundSelected++;
}
}

float selectAmount = min(foundSelected, foundNonSelected) / 25.0f;
out_var_SV_TARGET = vec4(color.x, color.y, color.z, color.w * selectAmount * 1.4f);
}
)";
    
    {
        GLuint shader = glCreateShader(GL_FRAGMENT_SHADER);
        int size = (int)strlen(outlineFragShader);
        glShaderSource(shader, 1, &outlineFragShader, &size);
        glCompileShader(shader);
        
        GLint status = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
        if(!status)
        {
            char info[512];
            glGetShaderInfoLog(shader, 512, NULL, info);
            Log("Outline Fragment Shader: %s", info);
        }
        
        glAttachShader(r->outlineProgram, shader);
    }
    
    glLinkProgram(r->outlineProgram);
    glGetProgramiv(r->outlineProgram, GL_LINK_STATUS, &success);
    if(!success)
    {
        char infoLog[512];
        glGetProgramInfoLog(r->outlineProgram, 512, NULL, infoLog);
        DebugMessage(infoLog);
    }
    
    r->outlineColorUniform = glGetUniformLocation(r->outlineProgram, "color");
    r->outlineTransformUniform = glGetUniformLocation(r->outlineProgram, "transform");
}

void R_BeginPass(Vec3 camPos, Quat camRot, float horizontalFOV, float nearClip, float farClip)
{
    Renderer* r = &renderer;
    
    int width, height;
    OS_GetClientAreaSize(&width, &height);
    glViewport(0, 0, width, height);
    float aspectRatio = (float)width / height;
    
    const float n   = nearClip;
    const float f   = farClip;
    const float fov = horizontalFOV;
    
    PerFrameUniforms u;
    u.world2View = transpose(World2ViewMatrix(camPos, camRot));
    u.view2Proj  = View2ProjMatrix(n, f, fov, aspectRatio);
    // Negate z axis because in OpenGL, -z is forward while in our coordinate system it's the other way around
    u.view2Proj.c3 *= -1;
    u.view2Proj.m43 = 1;
    
    // TODO: Apparently because we translate from hlsl it seems like matrices are actually
    // access in a row-major manner (like in hlsl), so i guess retranspose it back, for now.
    // Don't forget to just make everything row major
    u.view2Proj = transpose(u.view2Proj);
    
    u.viewPos   = camPos;
    r->perFrameUniforms = u;
    glNamedBufferSubData(r->frameUbo, 0, sizeof(u), &u);
    
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, r->frameUbo);
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, r->objUbo);
    
    // Preparing render
    glClearColor(0.12f, 0.3f, 0.25f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable( GL_BLEND );
    
#ifdef Development
    // Prepare framebuffers for mouse picking
    {
        glDeleteTextures(1, &r->mousePickingColor);
        glDeleteTextures(1, &r->mousePickingDepth);
        glDeleteFramebuffers(1, &r->mousePickingFbo);
        
        glGenTextures(1, &r->mousePickingColor);
        glGenTextures(1, &r->mousePickingDepth);
        glCreateFramebuffers(1, &r->mousePickingFbo);
        glBindFramebuffer(GL_FRAMEBUFFER, r->mousePickingFbo);
        
        glBindTexture(GL_TEXTURE_2D, r->mousePickingColor);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R32I, width, height, 0, GL_RGB_INTEGER, GL_UNSIGNED_INT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, r->mousePickingColor, 0);
        
        glBindTexture(GL_TEXTURE_2D, r->mousePickingDepth);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, r->mousePickingDepth, 0);
        
        GLint clearColor[4] = {-1, -1, -1, -1};
        glClearBufferiv(GL_COLOR, 0, clearColor);
        glClear(GL_DEPTH_BUFFER_BIT);
    }
    
    // Prepare framebuffers for selection
    {
        glDeleteTextures(1, &r->selectionColor);
        glDeleteTextures(1, &r->selectionDepth);
        glDeleteFramebuffers(1, &r->selectionFbo);
        
        glGenTextures(1, &r->selectionColor);
        glGenTextures(1, &r->selectionDepth);
        glCreateFramebuffers(1, &r->selectionFbo);
        glBindFramebuffer(GL_FRAMEBUFFER, r->selectionFbo);
        
        glBindTexture(GL_TEXTURE_2D, r->selectionColor);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8UI, width, height, 0, GL_RGB_INTEGER, GL_UNSIGNED_INT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, r->selectionColor, 0);
        
        glBindTexture(GL_TEXTURE_2D, r->selectionDepth);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, r->selectionDepth, 0);
        
        GLuint clearColor[4] = {0, 0, 0, 0};
        glClearBufferuiv(GL_COLOR, 0, clearColor);
        glClear(GL_DEPTH_BUFFER_BIT);
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
#endif
}

void R_DrawModelNoReload(Model* model, Mat4 transform, int id)
{
    if(!model) return;
    
    Renderer* r = &renderer;
    
    // Scale, rotation and then position
    PerObjectUniforms objUniforms = {0};
    objUniforms.model2World = transpose(transform);
    
    glNamedBufferSubData(r->objUbo, 0, sizeof(objUniforms), &objUniforms);
    
    for(int i = 0; i < model->meshes.len; ++i)
    {
        auto& mesh = model->meshes[i];
        
        // shaderProgram should be a material's property
        glUseProgram(model->program);
        
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, r->frameUbo);
        glBindBufferBase(GL_UNIFORM_BUFFER, 1, r->objUbo);
        glBindVertexArray(mesh.handle);
        
        glDrawElements(GL_TRIANGLES, (GLsizei)mesh.indices.len, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
}

#ifdef Development
void R_DrawModelEditor(Model* model, Mat4 transform, int id, bool selected)
{
    if(!model) return;
    
    Renderer* r = &renderer;
    
    // Scale, rotation and then position
    PerObjectUniforms objUniforms = {0};
    objUniforms.model2World = transpose(transform);
    
    glNamedBufferSubData(r->objUbo, 0, sizeof(objUniforms), &objUniforms);
    
    for(int i = 0; i < model->meshes.len; ++i)
    {
        auto& mesh = model->meshes[i];
        
        // shaderProgram should be a material's property
        glUseProgram(model->program);
        
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, r->frameUbo);
        glBindBufferBase(GL_UNIFORM_BUFFER, 1, r->objUbo);
        glBindVertexArray(mesh.handle);
        
        glDrawElements(GL_TRIANGLES, (GLsizei)mesh.indices.len, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
    
    if(id != -1)
    {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, r->mousePickingFbo);
        
        // Additional draw calls for mouse picking in editor
        for(int i = 0; i < model->meshes.len; ++i)
        {
            auto& mesh = model->meshes[i];
            
            glUseProgram(r->mousePickingProgram);
            
            glBindBufferBase(GL_UNIFORM_BUFFER, 0, r->frameUbo);
            glBindBufferBase(GL_UNIFORM_BUFFER, 1, r->objUbo);
            glBindVertexArray(mesh.handle);
            
            glUniform1i(r->mousePickingIdUniform, id);
            
            glDrawElements(GL_TRIANGLES, (GLsizei)mesh.indices.len, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }
        
        // Draw into selection buffer (for outlines)
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, r->selectionFbo);
        
        if(selected)
        {
            for(int i = 0; i < model->meshes.len; ++i)
            {
                auto& mesh = model->meshes[i];
                
                glUseProgram(r->selectionProgram);
                
                glBindBufferBase(GL_UNIFORM_BUFFER, 0, r->frameUbo);
                glBindBufferBase(GL_UNIFORM_BUFFER, 1, r->objUbo);
                glBindVertexArray(mesh.handle);
                
                glDrawElements(GL_TRIANGLES, (GLsizei)mesh.indices.len, GL_UNSIGNED_INT, 0);
                glBindVertexArray(0);
            }
        }
        
        // Bind back default framebuffer
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    }
}

void R_DrawSelectionOutlines(Vec4 color)
{
    Renderer* r = &renderer;
    
    Vec3 verts[] =
    {
        {-1, -1, -1}, {1, -1, -1}, {1, 1, -1},
        {-1, -1, -1}, {1, 1, -1}, {-1, 1, -1},
    };
    
    static GLuint vao = -1;
    if(vao == -1)
        glCreateVertexArrays(1, &vao);
    
    static GLuint vbo = -1;
    if(vbo == -1)
        glCreateBuffers(1, &vbo);
    
    // Disable writing to the depth buffer
    glDepthMask(GL_FALSE);
    
    glNamedBufferData(vbo, sizeof(verts), verts, GL_STATIC_DRAW);
    
    // Position
    glEnableVertexArrayAttrib(vao, 0);
    glVertexArrayAttribBinding(vao, 0, 0);
    glVertexArrayAttribFormat(vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
    
    glVertexArrayVertexBuffer(vao, 0, vbo, 0, sizeof(verts[0]));
    
    glUseProgram(r->outlineProgram);
    
    Mat4 identity = Mat4::identity;
    glUniformMatrix4fv(r->outlineTransformUniform, 1, false, (GLfloat*)&identity);
    glUniform4f(r->outlineColorUniform, color.x, color.y, color.z, color.w);
    
    glBindTexture(GL_TEXTURE_2D, r->selectionColor);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    
    // Reenable writing to the depth buffer
    glDepthMask(GL_TRUE);
}

#endif

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
    
    return shader;
}

R_Pipeline R_LinkShaders(Slice<R_Shader> shaders)
{
    R_Pipeline program = glCreateProgram();
    for(int i = 0; i < shaders.len; ++i)
        glAttachShader(program, shaders[i]);
    
    glLinkProgram(program);
    
    GLint status = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if(!status)
    {
        char info[512];
        glGetProgramInfoLog(program, 512, NULL, info);
        Log("%s", info);
    }
    
    return program;
}

void R_SetPipeline(R_Pipeline pipeline)
{
    glUseProgram(pipeline);
}

void R_SetUniformFloat(R_Pipeline pipeline, u32 binding, float value)
{
    GLuint globals = glGetUniformLocation(pipeline, "_Globals");
    
    glUniform1f(binding, value);
}

void R_SetUniformInt(R_Pipeline pipeline, u32 binding, int value)
{
    glUniform1i(binding, value);
}

void R_SetUniformVec3(R_Pipeline pipeline, u32 binding, Vec3 value)
{
    glUniform3fv(binding, 1, (float*)&value);
}

void R_SetUniformVec4(u32 binding, Vec4 value)
{
    glUniform4fv(binding, 1, (float*)&value);
}

void R_SetUniformMat4(u32 binding, Mat4 value)
{
    glUniformMatrix4fv(binding, 1, false, (float*)&value);
}

void R_SetUniformBuffer(u32 binding, void* buffer)
{
    
}

void R_Cleanup()
{
    
}

#ifdef Development
int R_ReadMousePickId(int xPos, int yPos)
{
    Renderer* r = &renderer;
    
    glBindTexture(GL_TEXTURE_2D, r->mousePickingColor);
    
    int width, height;
    int miplevel = 0;
    glGetTexLevelParameteriv(GL_TEXTURE_2D, miplevel, GL_TEXTURE_WIDTH, &width);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, miplevel, GL_TEXTURE_HEIGHT, &height);
    
    glBindFramebuffer(GL_READ_FRAMEBUFFER, r->mousePickingFbo);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    
    GLint pixel[4];
    GLint x = clamp(xPos, 0, width-1);
    GLint y = clamp(yPos, 0, height-1);
    glReadPixels(x, (height-1) - y, 1, 1, GL_RGB_INTEGER, GL_INT, pixel);
    glReadBuffer(GL_NONE);
    
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    
    return pixel[0];
}
#endif

void R_ImDrawArrow(Vec3 ori, Vec3 dst, float baseRadius, float coneLength, float coneRadius, Vec4 color);
void R_ImDrawCone(Vec3 baseCenter, Vec3 dir, float length, float radius, Vec4 color);

void R_ImDrawCylinder(Vec3 center, float radius, float height, Vec4 color)
{
    
}

void R_ImDrawQuad(Vec3 v1, Vec3 v2, Vec3 v3, Vec3 v4, Vec4 color)
{
    Renderer* r = &renderer;
    
    Vec3 verts[] =
    {
        v1, v2, v3,
        v1, v3, v4
    };
    
    static GLuint vao = -1;
    if(vao == -1)
        glCreateVertexArrays(1, &vao);
    
    static GLuint vbo = -1;
    if(vbo == -1)
        glCreateBuffers(1, &vbo);
    
    glNamedBufferData(vbo, sizeof(verts), verts, GL_STATIC_DRAW);
    
    // Position
    glEnableVertexArrayAttrib(vao, 0);
    glVertexArrayAttribBinding(vao, 0, 0);
    glVertexArrayAttribFormat(vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
    
    glVertexArrayVertexBuffer(vao, 0, vbo, 0, sizeof(verts[0]));
    
    glUseProgram(r->basicProgram);
    
    Mat4 world2Proj = r->perFrameUniforms.world2View * r->perFrameUniforms.view2Proj;
    glUniformMatrix4fv(r->transformUniform, 1, true, (GLfloat*)&world2Proj);
    glUniform4f(r->colorUniform, color.x, color.y, color.z, color.w);
    
    glBindVertexArray(vao);
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

void R_ImGuiShowDebugTextures()
{
    Renderer* r = &renderer;
    
    if(ImGui::CollapsingHeader("Mouse picking texture"))
    {
        Opengl_ImGuiShowTexture(r->mousePickingDepth);
    }
}
