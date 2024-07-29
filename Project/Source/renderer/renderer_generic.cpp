
#include "renderer_generic.h"

static Renderer renderer;

struct BasicMesh
{
    Slice<Vec3> verts;
    Slice<u32>  indices;
};

// Generate some basic meshes

// Generate unit cylinder with radius=0.5 and height=1.0
BasicMesh GenerateUnitCylinder()
{
    BasicMesh res = {0};
    constexpr int numPointsBase = 25;
    constexpr int numIndices = (numPointsBase - 2) * 2 + numPointsBase * 2;
    static Vec3 verts[numPointsBase * 2];
    static u32 indices[numIndices];
    for(int i = 0; i < numPointsBase; ++i)
    {
        
    }
    
    res.verts   = {.ptr=verts,   .len=numPointsBase * 2};
    res.indices = {.ptr=indices, .len=numIndices};
    return res;
}

// Generate unit cone with radius=0.5 and height=1.0
// (it points up)
BasicMesh GenerateUnitCone()
{
    return {0};
}

static const BasicMesh cylinder = GenerateUnitCylinder();
static const BasicMesh cone = GenerateUnitCone();

#ifdef GFX_OPENGL
#include "renderer/renderer_opengl.cpp"
#endif
#ifdef GFX_D3D11
#include "renderer/renderer_d3d11.cpp"
#endif

void R_DrawModel(Model* model, Mat4 transform, int id)
{
#ifdef Development
    
#endif
    
    R_DrawModelNoReload(model, transform, id);
}