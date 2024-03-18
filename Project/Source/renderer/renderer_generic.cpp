
#include "renderer_generic.h"

static Renderer renderer;

#ifdef OS_SupportOpenGL
#include "renderer/renderer_opengl.cpp"
#endif
#ifdef OS_SupportD3D11
#include "renderer/renderer_d3d11.cpp"
#endif

InitRenderer_Signature(InitRenderer_Stub) {}
Render_Signature(Render_Stub) {}
CreateGPUBuffers_Signature(CreateGPUBuffers_Stub) {}
RenderModelRelease_Signature(RenderModelRelease_Stub) {}
Cleanup_Signature(Cleanup_Stub) {}

InitRenderer_Type* InitRenderer = InitRenderer_Stub;
Render_Type* Render = Render_Stub;
CreateGPUBuffers_Type* CreateGPUBuffers = CreateGPUBuffers_Stub;
RenderModelRelease_Type* RenderModelRelease = RenderModelRelease_Stub;
Cleanup_Type* Cleanup = Cleanup_Stub;

void StubInitRenderer() {}
void StubRender(RenderSettings settings) {}
void StubCreateGPUBuffers(Model* model) {}
void StubRenderModel(Model* model) {}
void StubCleanup() {}

void SetRenderFunctionPointers(OS_GraphicsLib gfxLib)
{
    switch(gfxLib)
    {
        case GfxLib_None:
        {
            InitRenderer       = StubInitRenderer;
            CreateGPUBuffers   = StubCreateGPUBuffers;
            RenderModelRelease = StubRenderModel;
            Cleanup = StubCleanup;
            break;
        }
        case GfxLib_OpenGL:
        {
            InitRenderer       = gl_InitRenderer;
            CreateGPUBuffers   = StubCreateGPUBuffers;
            RenderModelRelease = gl_RenderModel;
            Cleanup = gl_Cleanup;
            TODO; // Missing create gpu buffers
            break;
        }
        case GfxLib_D3D11:
        {
            InitRenderer       = d3d11_InitRenderer;
            CreateGPUBuffers   = StubCreateGPUBuffers;
            RenderModelRelease = StubRenderModel;
            Cleanup = StubCleanup;
            TODO; // Missing a bunch of stuff
            break;
        }
        default:
        {
            TODO;
            OS_FatalError("This Graphics API is currently not supported.");
        }
    }
}

Model* LoadModelByName(const char* name)
{
    Model* model = LoadModelAssetByName(name);
    CreateGPUBuffers(model);
    return model;
}

Model* LoadModel(const char* path)
{
    Model* model = LoadModelAsset(path, nullptr);
    CreateGPUBuffers(model);
    return model;
}

void RenderModelDevelopment(Model* model)
{
    // Hot reload the model if necessary
    // if(relevant files changed) LoadModelByName()...
    TODO;
    
    RenderModelRelease(model);
}
