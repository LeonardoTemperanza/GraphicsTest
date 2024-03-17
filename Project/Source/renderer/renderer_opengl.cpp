
#include "base.h"
#include "renderer_generic.h"
#include "embedded_files.h"
#include "embedded_models.h"

void gl_InitRenderer()
{
    gl_Renderer* r = &renderer.glRenderer;
    memset(r, 0, sizeof(gl_Renderer));
    
    // This loading stuff should actually be moved to "load scene" or something
    
    // Allocate buffers
    glCreateVertexArrays(1, &r->vao);
    GLuint buffers[4];
    glCreateBuffers(3, buffers);
    r->vbo = buffers[0];
    //r->ebo = buffers[1];
    r->appUbo = buffers[1];
    r->frameUbo = buffers[2];
    
    //glNamedBufferData(r->vbo, sizeof(vertices), vertices, GL_STATIC_DRAW);
    //glNamedBufferData(r->ebo, sizeof(indices), indices, GL_STATIC_DRAW);
    //glNamedBufferData(r->appUbo, sizeof(PerAppUniforms), nullptr, GL_DYNAMIC_DRAW);
    glNamedBufferData(r->frameUbo, sizeof(PerFrameUniforms), nullptr, GL_DYNAMIC_DRAW);
    
#if 0
    // TODO: The binding needs to match the one in the shaders. There should be a common .h file used in both.
    glEnableVertexArrayAttrib(r->vao, 0);
    glVertexArrayAttribBinding(r->vao, 0, 0);
    glVertexArrayAttribFormat(r->vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
    
    glEnableVertexArrayAttrib(r->vao, 1);
    glVertexArrayAttribBinding(r->vao, 1, 0);
    glVertexArrayAttribFormat(r->vao, 1, 3, GL_FLOAT, GL_FALSE, 0);
    
    glVertexArrayVertexBuffer(r->vao, 0, r->vbo, 0, 6*sizeof(GLfloat));
    //glVertexArrayElementBuffer(r->vao, r->ebo);
#endif
    
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
}

static Arena rendererArena = ArenaVirtualMemInit(GB(4), MB(2));

struct MeshRenderInfo
{
    GLuint vao;
    GLuint vbo;
    GLuint vbo2;
    GLuint ebo;
};

Model* gl_LoadModel(const char* name)
{
    TODO;
    return nullptr;
    //LoadModelAsset();
}

static void gl_RenderModel(Model* model)
{
    gl_Renderer* r = &renderer.glRenderer;
    
    // This is test code
#if 0
    static Model* model = nullptr;
    static Slice<MeshRenderInfo> infos = {0};
    if(!model)
    {
        model = LoadModelAsset("Gun/Gun.model", &rendererArena);
        if(!model) return;
        
        infos.len = model->meshes.len;
        infos.ptr = ArenaAllocArray(MeshRenderInfo, infos.len, &rendererArena);
        
        for(int i = 0; i < infos.len; ++i)
        {
            auto& info = infos[i];
            auto& mesh = model->meshes[i];
            
            glCreateVertexArrays(1, &info.vao);
            GLuint bufferIds[2];
            glCreateBuffers(2, bufferIds);
            info.vbo = bufferIds[0];
            info.ebo = bufferIds[1];
            
            // Bind the vao and stuff
            glNamedBufferData(info.vbo, mesh.verts.len * sizeof(mesh.verts[0]), mesh.verts.ptr, GL_STATIC_DRAW);
            glNamedBufferData(info.ebo, mesh.indices.len * sizeof(mesh.indices[0]), mesh.indices.ptr, GL_STATIC_DRAW);
            
            glEnableVertexArrayAttrib(info.vao, 0);
            glVertexArrayAttribBinding(info.vao, 0, 0);
            glVertexArrayAttribFormat(info.vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
            
            glEnableVertexArrayAttrib(info.vao, 1);
            glVertexArrayAttribBinding(info.vao, 1, 0);
            glVertexArrayAttribFormat(info.vao, 1, 3, GL_FLOAT, GL_FALSE, 0);
            
            glVertexArrayVertexBuffer(info.vao, 0, info.vbo, 0, sizeof(mesh.verts[0]));
            //sglVertexArrayVertexBuffer(info.vao, 0, info.vbo2, 0, sizeof(mesh.normals[0]));
            glVertexArrayElementBuffer(info.vao, info.ebo);
        }
    }
    
    glUseProgram(r->shaderProgram);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    
    for(int i = 0; i < model->meshes.len; ++i)
    {
        auto& mesh = model->meshes[i];
        
        glBindVertexArray(infos[i].vao);
        glDrawElements(GL_TRIANGLES, mesh.indices.len, GL_UNSIGNED_INT, 0);
    }
#endif
}

#if 0
void gl_Render(RenderSettings settings)
{
    gl_Renderer* r = &renderer.glRenderer;
    
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
    //glDrawArrays(GL_TRIANGLES, 0, ArrayCount(vertices) / 2);
    
    //gl_RenderModel();
}
#endif

void gl_Cleanup()
{
    
}