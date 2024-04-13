
#include "base.h"
#include "renderer_generic.h"
#include "embedded_files.h"

// The structure here should be changed so that
// the renderer operates on large amounts of data
// at a time, because these are all function pointers,
// so it's not very fast to repeatedly call these in a loop

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
    u.world2View = World2ViewMatrix(camera.position, camera.rotation);
    u.view2Proj  = View2ProjMatrix(settings.nearClipPlane, settings.farClipPlane, settings.horizontalFOV, aspectRatio);
    u.viewPos    = camera.position;
    glNamedBufferSubData(r->frameUbo, 0, sizeof(u), &u);
    
    // Preparing render
    glClearColor(0.12f, 0.3f, 0.25f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    
    glUseProgram(r->shaderProgram);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
}

void Render(Slice<Entity> entities, RenderSettings renderSettings)
{
    Renderer* r = &renderer;
    auto& settings = renderSettings;
    
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
    u.world2View = World2ViewMatrix(camera.position, camera.rotation);
    u.view2Proj  = View2ProjMatrix(settings.nearClipPlane, settings.farClipPlane, settings.horizontalFOV, aspectRatio);
    u.viewPos    = camera.position;
    glNamedBufferSubData(r->frameUbo, 0, sizeof(u), &u);
    
    // Preparing render
    glClearColor(0.12f, 0.3f, 0.25f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    
    glUseProgram(r->shaderProgram);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    
    // Draw all models
    for(int i = 0; i < entities.len; ++i)
        R_DrawModel(entities[i].model, entities[i].pos, entities[i].rot, entities[i].scale);
}

void R_DrawModel(Model* model, Vec3 pos, Quat rot, Vec3 scale)
{
    if(!model) return;
    
    Renderer* r = &renderer;
    
    // Try to hot-reload if we're in development mode
#ifdef Development
    // Do some reloading stuff etc.
    MaybeReloadModelAsset(model);
#endif
    
    // Scale, rotation and then position
    PerObjectUniforms objUniforms = {0};
    objUniforms.model2World = Model2WorldMatrix(pos, rot, scale);
    
    glNamedBufferSubData(r->objUbo, 0, sizeof(objUniforms), &objUniforms);
    
    for(int i = 0; i < model->meshes.len; ++i)
    {
        auto& mesh = model->meshes[i];
        gl_MeshInfo* meshInfo = (gl_MeshInfo*)mesh.gfxInfo;
        
        // shaderProgram should be a material's property
        glUseProgram(r->shaderProgram);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        
        glUseProgram(r->shaderProgram);
        
        glBindVertexArray(meshInfo->vao);
        
#if 0
        auto material = mesh.material;
        if(material)
        {
            for(int i = 0; i < material->textures.len; ++i)
            {
                auto texture = material->textures[i];
                if(texture)
                {
                    auto texInfo = (gl_TextureInfo*)texture->gfxInfo;
                    glActiveTexture(GL_TEXTURE0 + i);
                    glBindTexture(GL_TEXTURE_2D, texInfo->objId);
                }
            }
        }
#endif
        
        glDrawElements(GL_TRIANGLES, mesh.indices.len, GL_UNSIGNED_INT, 0);
        
        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

void R_TransferModel(Model* model, Arena* arena)
{
    // Setup textures
#if 0
    for(int i = 0; i < model->meshes.len; ++i)
    {
        auto material = model->meshes[i].material;
        
        for(int j = 0; j < material->textures.len; ++j)
        {
            auto texture = material->textures[j];
            if(texture && !texture->allocatedOnGPU)
            {
                auto texInfo = ArenaZAllocTyped(gl_TextureInfo, arena);
                texture->gfxInfo = (void*)texInfo;
                
                GLuint texId;
                glGenTextures(1, &texId);
                texInfo->objId = texId;
                
                // TODO: Use bindless API?
                glBindTexture(GL_TEXTURE_2D, texId);
                {
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    
                    // TODO: Based on the number of channels, might want to change this
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texture->width, texture->height, 0, GL_RGB, GL_UNSIGNED_BYTE, texture->blob.ptr);
                    glGenerateMipmap(GL_TEXTURE_2D);
                }
                glBindTexture(GL_TEXTURE_2D, 0);
                
                texture->allocatedOnGPU = true;
            }
        }
    }
#endif
    
    // Setup meshes
    for(int i = 0; i < model->meshes.len; ++i)
    {
        auto& mesh = model->meshes[i];
        
        auto meshInfo = ArenaZAllocTyped(gl_MeshInfo, arena);
        mesh.gfxInfo = meshInfo;
        
        glCreateVertexArrays(1, &meshInfo->vao);
        GLuint bufferIds[2];
        glCreateBuffers(2, bufferIds);
        meshInfo->vbo = bufferIds[0];
        meshInfo->ebo = bufferIds[1];
        
        glNamedBufferData(meshInfo->vbo, mesh.verts.len * sizeof(mesh.verts[0]), mesh.verts.ptr, GL_STATIC_DRAW);
        glNamedBufferData(meshInfo->ebo, mesh.indices.len * sizeof(mesh.indices[0]), mesh.indices.ptr, GL_STATIC_DRAW);
        
        // Position
        glEnableVertexArrayAttrib(meshInfo->vao, 0);
        glVertexArrayAttribBinding(meshInfo->vao, 0, 0);
        glVertexArrayAttribFormat(meshInfo->vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
        
        // Normal
        glEnableVertexArrayAttrib(meshInfo->vao, 1);
        glVertexArrayAttribBinding(meshInfo->vao, 1, 0);
        glVertexArrayAttribFormat(meshInfo->vao, 1, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, normal));
        
        // Texture coords
        glEnableVertexArrayAttrib(meshInfo->vao, 2);
        glVertexArrayAttribBinding(meshInfo->vao, 2, 0);
        glVertexArrayAttribFormat(meshInfo->vao, 2, 2, GL_FLOAT, GL_FALSE, offsetof(Vertex, texCoord));
        
        // Tangents
        glEnableVertexArrayAttrib(meshInfo->vao, 3);
        glVertexArrayAttribBinding(meshInfo->vao, 3, 0);
        glVertexArrayAttribFormat(meshInfo->vao, 3, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, tangent));
        
        glVertexArrayVertexBuffer(meshInfo->vao, 0, meshInfo->vbo, 0, sizeof(mesh.verts[0]));
        glVertexArrayElementBuffer(meshInfo->vao, meshInfo->ebo);
    }
}

void R_Cleanup()
{
    
}
