
#include "renderer_backend/generic.h"

static Renderer renderer;

#ifdef GFX_OPENGL
#include "renderer_backend/renderer_opengl.cpp"
#elif defined(GFX_D3D11)
#include "renderer_backend/d3d11.cpp"
#else
#error "No gfx api selected from the implemented ones"
#endif

bool IsTextureFormatSigned(R_TextureFormat format)
{
    switch(format)
    {
        case R_TexNone:  return false;
        case R_TexR8:    return true;
        case R_TexRG8:   return true;
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
        case R_TexR8:    return false;
        case R_TexRG8:   return false;
        case R_TexRGBA8: return false;
        case R_TexR8I:   return true;
        case R_TexR8UI:  return true;
        case R_TexR32I:  return true;
    }
    
    return false;
}

// Generate some basic meshes

// Generate unit cylinder with radius=1.0 and height=1.0
R_Mesh GenerateUnitCylinderMesh()
{
    constexpr int numPointsBase = 25;
    constexpr int numVerts = numPointsBase * 2;
    constexpr int numTris = (numPointsBase - 2) * 2 + numPointsBase * 2;
    constexpr int numIndices = numTris * 3;
    static Vec3 verts[numVerts];
    static s32 indices[numIndices];
    
    const float height = 1.0f;
    const float radius = 1.0f;
    
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
    for(int i = 1; i < numPointsBase - 1; ++i)
    {
        indices[curIdx++] = 0;
        indices[curIdx++] = i+1;
        indices[curIdx++] = i+0;
    }
    
    // Top tris
    for(int i = numPointsBase + 1; i < numPointsBase * 2 - 1; ++i)
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
            int idx1 = (i+1)%numPointsBase;
            int idx2 = (i+1)%numPointsBase+numPointsBase;
            int idx3 = i;
            
            indices[curIdx++] = idx1;
            indices[curIdx++] = idx2;
            indices[curIdx++] = idx3;
        }
        
        // Tri 2 (edge on top)
        {
            int idx1 = (i+1)%numPointsBase + numPointsBase;
            int idx2 = i+numPointsBase;
            int idx3 = i;
            
            indices[curIdx++] = idx1;
            indices[curIdx++] = idx2;
            indices[curIdx++] = idx3;
        }
    }
    
    assert(curIdx == numIndices);
    
    return R_UploadBasicMesh(ArrToSlice(verts), {}, ArrToSlice(indices));
}

// Generate unit cone with radius=1.0f and height=1.0
// (it points up)
R_Mesh GenerateUnitConeMesh()
{
    constexpr int numPointsBase = 25;
    constexpr int numVerts = numPointsBase + 1;
    constexpr int numTris = numPointsBase - 2 + numPointsBase;
    constexpr int numIndices = numTris * 3;
    Vec3 verts[numVerts];
    s32 indices[numIndices];
    
    const float height = 1.0f;
    const float radius = 1.0f;
    
    // Vertices
    
    // Bottom base
    for(int i = 0; i < numPointsBase; ++i)
    {
        float angle = (float)i / numPointsBase * 2.0f * Pi;
        Vec3 p = {cos(angle) * radius, 0.0f, sin(angle) * radius};
        verts[i] = p;
    }
    
    // Top vertex
    verts[numPointsBase] = {0.0f, height, 0.0f};
    
    // Indices
    int curIdx = 0;
    
    // Bottom tris
    for(int i = 1; i < numPointsBase - 1; ++i)
    {
        indices[curIdx++] = 0;
        indices[curIdx++] = i+1;
        indices[curIdx++] = i+0;
    }
    
    // Lateral tris
    
    for(int i = 0; i < numPointsBase; ++i)
    {
        int idx1 = (i+1)%numPointsBase;
        int idx2 = numPointsBase;
        int idx3 = i;
        
        indices[curIdx++] = idx1;
        indices[curIdx++] = idx2;
        indices[curIdx++] = idx3;
    }
    
    assert(curIdx == numIndices);
    
    return R_UploadBasicMesh(ArrToSlice(verts), {}, ArrToSlice(indices));
}

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
            case Uniform_Vec4:  size = 16; align = 16; break;
            case Uniform_Mat4:  size = 64; align = 16; break;
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

void R_DrawArrow(Vec3 ori, Vec3 dst, float baseRadius, float coneRadius, float coneLength)
{
    Vec3 diff = dst - ori;
    float length = magnitude(diff);
    Vec3 dir = normalize(diff);
    R_DrawCylinder(ori, dir, baseRadius, length);
    R_DrawCone(dst, dir, coneRadius, coneLength);
}

void R_SetToDefaultState()
{
    // Default renderer settings
    
    R_DepthTest(true);
    R_CullFace(true);
    R_AlphaBlending(true);
}
