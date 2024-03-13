
#include "base.h"
#include "renderer_opengl.h"
#include "embedded_files.h"
#include "embedded_models.h"

// Cube
float vertices[] = {
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
    0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 
    0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 
    0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 
    -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 
    
    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
    0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
    0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
    0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
    
    -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
    
    0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
    0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
    0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
    0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
    0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
    0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
    
    -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
    0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
    0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
    0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
    
    -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
    0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
    0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
    0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
    -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
    -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
    
    
    -0.5f+2.0f, -0.5f+2.0f, -0.5f+2.0f,  0.0f,  0.0f, -1.0f,
    0.5f+2.0f, -0.5f+2.0f, -0.5f+2.0f,  0.0f,  0.0f, -1.0f, 
    0.5f+2.0f,  0.5f+2.0f, -0.5f+2.0f,  0.0f,  0.0f, -1.0f, 
    0.5f+2.0f,  0.5f+2.0f, -0.5f+2.0f,  0.0f,  0.0f, -1.0f, 
    -0.5f+2.0f,  0.5f+2.0f, -0.5f+2.0f,  0.0f,  0.0f, -1.0f, 
    -0.5f+2.0f, -0.5f+2.0f, -0.5f+2.0f,  0.0f,  0.0f, -1.0f, 
    
    -0.5f+2.0f, -0.5f+2.0f,  0.5f+2.0f,  0.0f,  0.0f, 1.0f,
    0.5f+2.0f, -0.5f+2.0f,  0.5f+2.0f,  0.0f,  0.0f, 1.0f,
    0.5f+2.0f,  0.5f+2.0f,  0.5f+2.0f,  0.0f,  0.0f, 1.0f,
    0.5f+2.0f,  0.5f+2.0f,  0.5f+2.0f,  0.0f,  0.0f, 1.0f,
    -0.5f+2.0f,  0.5f+2.0f,  0.5f+2.0f,  0.0f,  0.0f, 1.0f,
    -0.5f+2.0f, -0.5f+2.0f,  0.5f+2.0f,  0.0f,  0.0f, 1.0f,
    
    -0.5f+2.0f,  0.5f+2.0f,  0.5f+2.0f, -1.0f,  0.0f,  0.0f,
    -0.5f+2.0f,  0.5f+2.0f, -0.5f+2.0f, -1.0f,  0.0f,  0.0f,
    -0.5f+2.0f, -0.5f+2.0f, -0.5f+2.0f, -1.0f,  0.0f,  0.0f,
    -0.5f+2.0f, -0.5f+2.0f, -0.5f+2.0f, -1.0f,  0.0f,  0.0f,
    -0.5f+2.0f, -0.5f+2.0f,  0.5f+2.0f, -1.0f,  0.0f,  0.0f,
    -0.5f+2.0f,  0.5f+2.0f,  0.5f+2.0f, -1.0f,  0.0f,  0.0f,
    
    0.5f+2.0f,  0.5f+2.0f,  0.5f+2.0f,  1.0f,  0.0f,  0.0f,
    0.5f+2.0f,  0.5f+2.0f, -0.5f+2.0f,  1.0f,  0.0f,  0.0f,
    0.5f+2.0f, -0.5f+2.0f, -0.5f+2.0f,  1.0f,  0.0f,  0.0f,
    0.5f+2.0f, -0.5f+2.0f, -0.5f+2.0f,  1.0f,  0.0f,  0.0f,
    0.5f+2.0f, -0.5f+2.0f,  0.5f+2.0f,  1.0f,  0.0f,  0.0f,
    0.5f+2.0f,  0.5f+2.0f,  0.5f+2.0f,  1.0f,  0.0f,  0.0f,
    
    -0.5f+2.0f, -0.5f+2.0f, -0.5f+2.0f,  0.0f, -1.0f,  0.0f,
    0.5f+2.0f, -0.5f+2.0f, -0.5f+2.0f,  0.0f, -1.0f,  0.0f,
    0.5f+2.0f, -0.5f+2.0f,  0.5f+2.0f,  0.0f, -1.0f,  0.0f,
    0.5f+2.0f, -0.5f+2.0f,  0.5f+2.0f,  0.0f, -1.0f,  0.0f,
    -0.5f+2.0f, -0.5f+2.0f,  0.5f+2.0f,  0.0f, -1.0f,  0.0f,
    -0.5f+2.0f, -0.5f+2.0f, -0.5f+2.0f,  0.0f, -1.0f,  0.0f,
    
    -0.5f+2.0f,  0.5f+2.0f, -0.5f+2.0f,  0.0f,  1.0f,  0.0f,
    0.5f+2.0f,  0.5f+2.0f, -0.5f+2.0f,  0.0f,  1.0f,  0.0f,
    0.5f+2.0f,  0.5f+2.0f,  0.5f+2.0f,  0.0f,  1.0f,  0.0f,
    0.5f+2.0f,  0.5f+2.0f,  0.5f+2.0f,  0.0f,  1.0f,  0.0f,
    -0.5f+2.0f,  0.5f+2.0f,  0.5f+2.0f,  0.0f,  1.0f,  0.0f,
    -0.5f+2.0f,  0.5f+2.0f, -0.5f+2.0f,  0.0f,  1.0f,  0.0f
};

gl_Renderer* gl_InitRenderer(Arena* permArena)
{
    auto r = (gl_Renderer*)malloc(sizeof(gl_Renderer));
    memset(r, 0, sizeof(gl_Renderer));
    
    // Allocate buffers
    glCreateVertexArrays(1, &r->vao);
    GLuint buffers[4];
    glCreateBuffers(3, buffers);
    r->vbo = buffers[0];
    //r->ebo = buffers[1];
    r->appUbo = buffers[1];
    r->frameUbo = buffers[2];
    
    glNamedBufferData(r->vbo, sizeof(vertices), vertices, GL_STATIC_DRAW);
    //glNamedBufferData(r->ebo, sizeof(indices), indices, GL_STATIC_DRAW);
    //glNamedBufferData(r->appUbo, sizeof(PerAppUniforms), nullptr, GL_DYNAMIC_DRAW);
    glNamedBufferData(r->frameUbo, sizeof(PerFrameUniforms), nullptr, GL_DYNAMIC_DRAW);
    
    // TODO: The binding needs to match the one in the shaders. There should be a common .h file used in both.
    glEnableVertexArrayAttrib(r->vao, 0);
    glVertexArrayAttribBinding(r->vao, 0, 0);
    glVertexArrayAttribFormat(r->vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayVertexBuffer(r->vao, 0, r->vbo, 0, 6*sizeof(GLfloat));
    
    glEnableVertexArrayAttrib(r->vao, 1);
    glVertexArrayAttribBinding(r->vao, 1, 0);
    glVertexArrayAttribFormat(r->vao, 1, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayVertexBuffer(r->vao, 0, r->vbo, 0, 6*sizeof(GLfloat));
    
    //glVertexArrayElementBuffer(r->vao, r->ebo);
    
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
    
    r->perFrameUniformIdx = glGetUniformBlockIndex(r->shaderProgram, "PerFrame");
    // @hack On my laptop (Redmibook 14 AMD Windows 11) this returns -1
    // for some reason... I guess if this happens just set it to 0
    // and just hope for the best?
    if(r->perFrameUniformIdx == -1) r->perFrameUniformIdx = 0;
    
    glBindBufferBase(GL_UNIFORM_BUFFER, perFrameBindingPoint, r->frameUbo);
    glUniformBlockBinding(r->shaderProgram, r->perFrameUniformIdx, perFrameBindingPoint);
    return r;
}

static Arena rendererArena = ArenaVirtualMemInit(GB(4), MB(2));

static void gl_RenderModel()
{
    // This is test code
    static Model* model = nullptr;
    if(!model)
    {
        model = LoadModel("W:/GraphicsTest/Assets/Raptoid/Raptoid.model", &rendererArena);
    }
    
    // Use the model, render it with
}

void gl_Render(gl_Renderer* r, RenderSettings settings)
{
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
    
    // Model is assumed to be already in world space.
    // World->View, followed by View->Projection
    
    PerFrameUniforms u;
    u.world2View = World2ViewMatrix(camera.position, camera.rotation);
    // Positive Z = forward in this coordinate system, and in opengl it's the opposite.
    // So we scale z by -1.0f, after having applied all the other transformations
    u.world2View.m31 *= -1.0f;
    u.world2View.m32 *= -1.0f;
    u.world2View.m33 *= -1.0f;
    u.world2View.m34 *= -1.0f;
    
    u.view2Proj = View2ProjMatrix(settings.nearClipPlane, settings.farClipPlane, settings.horizontalFOV, aspectRatio);
    glNamedBufferSubData(r->frameUbo, 0, sizeof(u), &u);
    
    glClearColor(0.12f, 0.3f, 0.25f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    
    glUseProgram(r->shaderProgram);
    glBindVertexArray(r->vao);
    glEnable(GL_DEPTH_TEST);
    //glEnable(GL_CULL_FACE);
    glDrawArrays(GL_TRIANGLES, 0, ArrayCount(vertices) / 2);
    
    gl_RenderModel();
}
