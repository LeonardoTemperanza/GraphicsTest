
#include "collision.h"
#include "base.h"

Ray CameraRay(int screenX, int screenY, Vec3 pos, Quat rot, float horizontalFOV)
{
    int width, height;
    OS_GetClientAreaSize(&width, &height);
    
    float aspectRatio = (float)width / height;
    
    // To [-1, 1]
    float x = 2.0f * screenX / width - 1.0f;
    float y = 1.0f - 2.0f * screenY / height;
    
    float fovRad = Deg2Rad(horizontalFOV);
    
    float tanHalfFov = std::tan(fovRad / 2.0f);
    float cameraX = x * tanHalfFov;
    float cameraY = y * tanHalfFov / aspectRatio;
    Vec3 cameraDir = {.x=cameraX, .y=cameraY, .z=1.0f};
    cameraDir = normalize(cameraDir);
    
    Vec3 worldDir = rot * cameraDir;
    worldDir = normalize(worldDir);
    return {.ori=pos, .dir=worldDir};
}

Ray TransformRay(Ray ray, Quat rot)
{
    // TODO
    TODO;
    return ray;
}

bool RayBoxIntersection(Ray ray, Quat rot, Aabb local)
{
    Quat inv = inverse(rot);
    ray = TransformRay(ray, inv);
    return RayAabbIntersection(ray, local);
}

// From: https://tavianator.com/2011/ray_box.html
bool RayAabbIntersection(Ray ray, Aabb aabb)
{
    double tx1 = (aabb.min.x - ray.ori.x) / ray.dir.x;
    double tx2 = (aabb.max.x - ray.ori.x) / ray.dir.x;
    
    double tmin = min(tx1, tx2);
    double tmax = max(tx1, tx2);
    
    double ty1 = (aabb.min.y - ray.ori.y) / ray.dir.y;
    double ty2 = (aabb.max.y - ray.ori.y) / ray.dir.y;
    
    tmin = max(tmin, min(ty1, ty2));
    tmax = min(tmax, max(ty1, ty2));
    
    double tz1 = (aabb.min.z - ray.ori.z) / ray.dir.z;
    double tz2 = (aabb.max.z - ray.ori.z) / ray.dir.z;
    
    tmin = max(tmin, min(tz1, tz2));
    tmax = min(tmax, max(tz1, tz2));
    
    return tmax >= tmin;
}

float RayPlaneDst(Ray ray, Vec3 p, Vec3 normal)
{
    float denom = dot(normal, ray.dir);
    if (std::abs(denom) > 1e-6)
    {
        Vec3 diff = p - ray.ori;
        float t = dot(diff, normal) / denom;
        // Intersection must be in front of ray
        if (t >= 0)
            return t;
    }
    
    // Ray is parallel to the plane, or
    // intersection is behind the ray; which
    // results in a missed intersection
    return FLT_MAX;
}