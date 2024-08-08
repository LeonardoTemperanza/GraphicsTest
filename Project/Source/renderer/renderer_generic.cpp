
#include "renderer_generic.h"

static Renderer renderer;

bool IsTextureFormatSigned(R_TextureFormat format)
{
    switch(format)
    {
        case R_TexNone:  return false;
        case R_TexRGB8:  return true;
        case R_TexRGBA8: return true;
        case R_TexR8I:   return true;
        case R_TexR8UI:  return false;
        case R_TexR32I:  return true;
    }
    
    return false;
}

bool IsTextureFormatInteger(R_TextureFormat format)
{
    switch(format)
    {
        case R_TexNone:  return false;
        case R_TexRGB8:  return false;
        case R_TexRGBA8: return false;
        case R_TexR8I:   return true;
        case R_TexR8UI:  return true;
        case R_TexR32I:  return true;
    }
    
    return false;
}

// Generate some basic meshes

// Generate unit cylinder with radius=1.0 and height=1.0
BasicMesh GenerateUnitCylinder()
{
    constexpr int numPointsBase = 25;
    constexpr int numIndices = (numPointsBase - 2) * 2 + numPointsBase * 2;
    static Vec3 verts[numPointsBase * 2];
    static u32 indices[numIndices];
    static bool cached = false;
    
    const float height = 1.0f;
    const float radius = 1.0f;
    
    BasicMesh res = {0};
    res.verts   = {.ptr=verts,   .len=numPointsBase * 2};
    res.indices = {.ptr=indices, .len=numIndices};
    if(cached) return res;
    
    // Vertices
    
    // Bottom base
    for(int i = 0; i < numPointsBase; ++i)
    {
        float angle = (float)i / numPointsBase * 2.0f * Pi;
        Vec3 p = {cos(angle) * radius, 0.0f, sin(angle) * radius};
        verts[i] = p;
    }
    
    // Top base
    for(int i = 0; i < numPointsBase; ++i)
    {
        float angle = (float)i / numPointsBase * 2.0f * Pi;
        Vec3 p = {cos(angle) * radius, height, sin(angle) * radius};
        verts[i + numPointsBase] = p;
    }
    
    // Indices
    int curIdx = 0;
    
    // Bottom tris
    for(int i = 1; i < numPointsBase; ++i)
    {
        indices[curIdx++] = 0;
        indices[curIdx++] = i+0;
        indices[curIdx++] = i+1;
    }
    
    // Top tris
    for(int i = numPointsBase + 1; i < numPointsBase * 2; ++i)
    {
        indices[curIdx++] = numPointsBase;
        indices[curIdx++] = i+0;
        indices[curIdx++] = i+1;
    }
    
    // Lateral tris
    for(int i = 0; i < numPointsBase; ++i)
    {
        // Tri 1 (edge on bottom)
        {
            int idx1 = i+0;
            int idx2 = i+1;
            int idx3 = (i+numPointsBase)+1;
            
            indices[curIdx++] = idx1;
            indices[curIdx++] = idx2;
            indices[curIdx++] = idx3;
        }
        
        // Tri 2 (edge on top)
        {
            int idx1 = (i+numPointsBase)+1;
            int idx2 = (i+numPointsBase)+0;
            int idx3 = i+0;
            
            indices[curIdx++] = idx1;
            indices[curIdx++] = idx2;
            indices[curIdx++] = idx3;
        }
    }
    
    cached = true;
    return res;
}

// Generate unit cone with radius=1.0f and height=1.0
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