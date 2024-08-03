
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

R_UniformValue MakeUniformFloat(float value)
{
    R_UniformValue u = {0};
    u.type = Uniform_Float;
    u.value.f = value;
    return u;
}

R_UniformValue MakeUniformInt(int value)
{
    R_UniformValue u = {0};
    u.type = Uniform_Int;
    u.value.i = value;
    return u;
}

R_UniformValue MakeUniformUInt(u32 value)
{
    R_UniformValue u = {0};
    u.type = Uniform_UInt;
    u.value.ui = value;
    return u;
}

R_UniformValue MakeUniformVec3(Vec3 value)
{
    R_UniformValue u = {0};
    u.type = Uniform_Vec3;
    u.value.v3 = value;
    return u;
}

R_UniformValue MakeUniformVec4(Vec4 value)
{
    R_UniformValue u = {0};
    u.type = Uniform_Vec4;
    u.value.v4 = value;
    return u;
}

R_UniformValue MakeUniformMat4(Mat4 value)
{
    R_UniformValue u = {0};
    u.type = Uniform_Mat4;
    u.value.mat4 = value;
    return u;
}

Slice<uchar> MakeUniformBufferStd140(Slice<R_UniformValue> desc, Arena* dst)
{
    // TODO: Triple check that this is correct
    Slice<uchar> res = {0};
    for(int i = 0; i < desc.len; ++i)
    {
        int size = 0;
        int align = 1;
        switch(desc[i].type)
        {
            case Uniform_None:  break;
            case Uniform_Count: break;
            case Uniform_Int:   size = 4;  align = 4;  break;
            case Uniform_UInt:  size = 4;  align = 4;  break;
            case Uniform_Float: size = 4;  align = 4;  break;
            case Uniform_Vec3:  size = 12; align = 16; break;
            case Uniform_Vec4:  size = 16; align = 16;  break;
            case Uniform_Mat4:  size = 64; align = 16;  break;
        }
        
        // NOTE TODO: This relies on the fact that arena allocation is always
        // contiguous. Maybe it would be best to use a different approach here
        auto ptr = (uchar*)ArenaAllocAndCopy(dst, &desc[i].value, size, align);
        if(i == 0)
        {
            res.ptr = ptr;
        }
        if(i == desc.len - 1)
        {
            res.len = size + ptr - res.ptr;
        }
    }
    
    return res;
}

#ifdef GFX_OPENGL
#include "renderer/renderer_opengl.cpp"
#endif
#ifdef GFX_D3D11
#include "renderer/renderer_d3d11.cpp"
#endif

void R_DrawModel(Model* model)
{
#ifdef Development
    
#endif
    
    R_DrawModelNoReload(model);
}