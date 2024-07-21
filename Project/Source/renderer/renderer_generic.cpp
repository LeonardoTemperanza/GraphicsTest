
#include "renderer_generic.h"

static Renderer renderer;

#ifdef GFX_OPENGL
#include "renderer/renderer_opengl.cpp"
#endif
#ifdef GFX_D3D11
#include "renderer/renderer_d3d11.cpp"
#endif

void R_DrawModel(Model* model, Mat4 transform)
{
#ifdef Development
    
#endif
    
    R_DrawModelNoReload(model, transform);
}