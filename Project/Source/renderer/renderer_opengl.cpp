
#include "renderer_opengl.h"
#include "embedded_files.h"
#include "embedded_models.h"

// Cube
const float vertices[] =
{
    -0.5f, -0.5f, -0.5f,
    +0.5f, -0.5f, -0.5f,
    +0.5f, +0.5f, -0.5f,
    -0.5f, +0.5f, -0.5f,
    -0.5f, -0.5f, +0.5f,
    +0.5f, -0.5f, +0.5f,
    +0.5f, +0.5f, +0.5f,
    -0.5f, +0.5f, +0.5f,
};

const unsigned int indices[] =
{
    // Back
    2, 1, 0,
    2, 0, 3,
    // Front
    4, 5, 6,
    4, 6, 7,
    // Bottom
    0, 1, 5,
    0, 5, 4,
    // Top
    2, 3, 7,
    2, 7, 6,
    // Left
    0, 4, 7,
    0, 7, 3,
    // Right
    1, 6, 5,
    1, 2, 6,
};

gl_Renderer gl_InitRenderer(Arena* permArena)
{
    gl_Renderer r = {0};
    
    // Allocate buffers
    glCreateVertexArrays(1, &r.vao);
    GLuint buffers[4];
    glCreateBuffers(4, buffers);
    r.vbo = buffers[0];
    r.ebo = buffers[1];
    r.appUbo = buffers[2];
    r.frameUbo = buffers[3];
    
    glNamedBufferData(r.vbo, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glNamedBufferData(r.ebo, sizeof(indices), indices, GL_STATIC_DRAW);
    //glNamedBufferData(r.appUbo, sizeof(PerAppUniforms), nullptr, GL_DYNAMIC_DRAW);
    glNamedBufferData(r.frameUbo, sizeof(PerFrameUniforms), nullptr, GL_DYNAMIC_DRAW);
    
    // TODO: The binding needs to match the one in the shaders. There should be a common .h file used in both.
    glEnableVertexArrayAttrib(r.vao, 0);
    glVertexArrayAttribBinding(r.vao, 0, 0);
    glVertexArrayAttribFormat(r.vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayVertexBuffer(r.vao, 0, r.vbo, 0, 3*sizeof(GLfloat));
    
    glVertexArrayElementBuffer(r.vao, r.ebo);
    
    // Specialize and link SPIR-V shader
    r.vertShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderBinary(1, &r.vertShader, GL_SHADER_BINARY_FORMAT_SPIR_V, vertShader, sizeof(vertShader));
    glSpecializeShader(r.vertShader, "main", 0, nullptr, nullptr);
    
    r.fragShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderBinary(1, &r.fragShader, GL_SHADER_BINARY_FORMAT_SPIR_V, fragShader, sizeof(fragShader));
    glSpecializeShader(r.fragShader, "main", 0, nullptr, nullptr);
    
    r.shaderProgram = glCreateProgram();
    glAttachShader(r.shaderProgram, r.vertShader);
    glAttachShader(r.shaderProgram, r.fragShader);
    glLinkProgram(r.shaderProgram);
    
    // TODO: Do uniforms really have to be dynamically queried in opengl 4.6?
    int perFrameBindingPoint = 0;
    r.perFrameUniformIdx = glGetUniformBlockIndex(r.shaderProgram, "PerFrame");
    glBindBufferBase(GL_UNIFORM_BUFFER, perFrameBindingPoint, r.frameUbo);
    glUniformBlockBinding(r.shaderProgram, r.perFrameUniformIdx, perFrameBindingPoint);
    
    return r;
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
    
    // Apply quaternion rotation:
    // 2*(q0^2 + q1^2) - 1    2*(q1*q2 - q0*q3)      2*(q1*q3 + q0*q2)
    // 2*(q1*q2 + q0*q3)      2*(q0^2 + q2^2) - 1    2*(q2*q3 - q0*q1)
    // 2*(q1*q3 - q0*q2)      2*(q2*q3 + q0*q1)      2*(q0^2 + q3^2) - 1
    
    // This is the View->Projection matrix
    float view2Proj[4][4] =
    {
        { n/right, 0,     0,            0  },
        { 0,       n/top, 0,            0  },
        { 0,       0,     -(f+n)/(f-n), -1 },
        { 0,       0,     -2*f*n/(f-n), 0  }
    };
    
    // World to view
#if 0
    float a[4][4] = {0};
    a[0][0] = 1 - 2*rot.y*rot.y - 2*rot.z*rot.z;
    a[0][1] = 2*rot.x*rot.y + 2*rot.w*rot.z;
    a[0][2] = 2*rot.x*rot.z - 2*rot.w*rot.w;
    a[1][0] = 2*rot.x*rot.y - 2*rot.w*rot.z;
    a[1][1] = 1 - 2*rot.x*rot.x - 2*rot.z*rot.z;
    a[1][2] = 2*rot.y*rot.z + 2*rot.w*rot.x;
    a[2][0] = 2*rot.x*rot.z + 2*rot.w*rot.y;
    a[2][1] = 2*rot.y*rot.z - 2*rot.w*rot.x;
    a[2][2] = 1 - 2*rot.x*rot.x - 2*rot.y*rot.y;
    a[3][0] = -pos.x*a[0][0] -pos.y*a[1][0] +pos.z*a[2][0];
    a[3][1] = -pos.x*a[0][1] -pos.y*a[1][1] +pos.z*a[2][1];
    a[3][2] = -pos.x*a[0][2] -pos.y*a[1][2] +pos.z*a[2][2];
    a[3][3] = 1;
#endif
    
#if 0
    float world2View[4][4] =
    {
        //
        //
        //{ -pos.x*x.x -pos.y*y.x + pos.z*z.x, -pos.x*x.y -pos.y*y.y + pos.z*z.y, -pos.x*x.z -pos.y*y.z + pos.z*z.z, 1 }
        { -pos.x, -pos.y, pos.z, 1 }
    };
#endif
    
    PerFrameUniforms u;
    u.world2View = World2ViewMatrix(camera.position, camera.rotation);
    u.view2Proj  = View2ProjMatrix(settings.nearClipPlane, settings.farClipPlane, settings.horizontalFOV, aspectRatio);
    glNamedBufferSubData(r->frameUbo, 0, sizeof(u), &u);
    
    glClearColor(0.12f, 0.3f, 0.25f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    
    glUseProgram(r->shaderProgram);
    glBindVertexArray(r->vao);
    glEnable(GL_CULL_FACE);
    glDrawElements(GL_TRIANGLES, ArrayCount(indices), GL_UNSIGNED_INT, nullptr);
}
