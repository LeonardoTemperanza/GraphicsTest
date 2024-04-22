
#include "renderer_generic.h"

static Renderer renderer;

#ifdef GFX_OPENGL
#include "renderer/renderer_opengl.cpp"
#endif
#ifdef GFX_DX3D11
#include "renderer/renderer_d3d11.cpp"
#endif

void R_DrawModel(Model* model, Vec3 pos, Quat rot, Vec3 scale)
{
#ifdef Development
    
#endif
    
    R_DrawModelNoReload(model, pos, rot, scale);
}