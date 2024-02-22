
#pragma once

struct Particle;

struct RenderSettings
{
    Transform camera;
    float horizontalFOV;
    float nearClipPlane;
    float farClipPlane;
    
    Slice<Particle> particles;
};

struct Primitives
{
    Slice<float> sphereVerts;
    Slice<u32> sphereIndices;
};

Primitives GeneratePrimitives(Arena* dst);
