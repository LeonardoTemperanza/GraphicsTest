
#include "renderer_generic.h"

// Mhmmm, virtual functions honestly seem better in this specific
// case... How do C people do this properly?

Renderer* StubInitRenderer(Arena* arena) { return nullptr; }
void StubRender(Renderer* r, RenderSettings settings) {};

void SetRenderFunctionPointers(OS_GraphicsLib gfxLib)
{
#define CastToGenericInit   (Renderer* (*)(Arena*))
#define CastToGenericRender (void (*)(Renderer* r, RenderSettings settings))
    switch(gfxLib)
    {
        case GfxLib_None:
        {
            InitRenderer = StubInitRenderer;
            Render = StubRender;
            break;
        }
        case GfxLib_OpenGL:
        {
            InitRenderer = CastToGenericInit gl_InitRenderer;
            Render = CastToGenericRender gl_Render;
            break;
        }
        case GfxLib_D3D11:
        {
            InitRenderer = CastToGenericInit d3d11_InitRenderer;
            Render = CastToGenericRender d3d11_Render;
            break;
        }
        default:
        {
            TODO;
            OS_FatalError("This Graphics API is currently not supported.");
        }
    }
#undef CastToGenericInit
#undef CastToGenericRender
}