
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
SetupGPUResources_Signature(SetupGPUResources_Stub) {}
RenderModelRelease_Signature(RenderModelRelease_Stub) {}
Cleanup_Signature(Cleanup_Stub) {}

InitRenderer_Type* InitRenderer = InitRenderer_Stub;
Render_Type* Render = Render_Stub;
SetupGPUResources_Type* SetupGPUResources = SetupGPUResources_Stub;
RenderModelRelease_Type* RenderModelRelease = RenderModelRelease_Stub;
Cleanup_Type* Cleanup = Cleanup_Stub;

void SetRenderFunctionPointers(OS_GraphicsLib gfxLib)
{
    switch(gfxLib)
    {
        case GfxLib_None:
        {
            InitRenderer       = InitRenderer_Stub;
            Render             = Render_Stub;
            SetupGPUResources  = SetupGPUResources_Stub;
            Cleanup            = Cleanup_Stub;
            break;
        }
        case GfxLib_OpenGL:
        {
            InitRenderer       = gl_InitRenderer;
            Render             = gl_Render;
            SetupGPUResources  = gl_SetupGPUResources;
            Cleanup            = gl_Cleanup;
            break;
        }
        case GfxLib_D3D11:
        {
            InitRenderer       = d3d11_InitRenderer;
            Render             = d3d11_Render;
            Cleanup            = Cleanup_Stub;
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
    //CreateGPUBuffers(model);
    return model;
}

Model* LoadModel(const char* path)
{
    Model* model = LoadModelAsset(path);
    //CreateGPUBuffers(model);
    return model;
}

void RenderModelDevelopment(Model* model)
{
    // Hot reload the model if necessary
    // if(relevant files changed) LoadModelByName()...
    TODO;
    
    RenderModelRelease(model);
}
