
#pragma once

#include "base.h"

struct Ray
{
    Vec3 ori;
    Vec3 dir;
};

struct Aabb
{
    Vec3 min;
    Vec3 max;
};

// Ray manipulation
Ray CameraRay(int screenX, int screenY, Vec3 pos, Quat rot, float horizontalFov);
Ray TransformRay(Ray ray, Quat rot);

// Raycast queries.
// When distance is returned, distance = FLT_MAX means that no object was hit
bool RayBoxIntersection(Ray ray, Quat rot, Aabb local);
bool RayAabbIntersection(Ray ray, Aabb aabb);
float RayPlaneDst(Ray ray, Vec3 p, Vec3 normal);
