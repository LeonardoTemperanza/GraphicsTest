
// NOTE: @incomplete Things that do not work right now:
// Incomplete Matrix stuff
// List operations with macros are not implemented (maybe they're not needed in this project)

#include "base.h"

const Vec3 Vec3::right    = { 1.0f,  0.0f,  0.0f};
const Vec3 Vec3::up       = { 0.0f,  1.0f,  0.0f};
const Vec3 Vec3::forward  = { 0.0f,  0.0f,  1.0f};
const Vec3 Vec3::left     = {-1.0f,  0.0f,  0.0f};
const Vec3 Vec3::down     = { 0.0f, -1.0f,  0.0f};
const Vec3 Vec3::backward = { 0.0f,  0.0f, -1.0f};

const Quat Quat::identity = { .w=1.0f, .x=0.0f, .y=0.0f, .z=0.0f };

// Vector and matrix operations
Vec3 operator +(Vec3 a, Vec3 b)
{
    return {.x = a.x+b.x, .y = a.y+b.y, .z = a.z+b.z};
}

Vec3 operator -(Vec3 a, Vec3 b)
{
    return {.x = a.x-b.x, .y = a.y-b.y, .z = a.z-b.z};
}

Vec3 operator *(Vec3 v, float f)
{
    return {.x = v.x*f, .y = v.y*f, .z = v.z*f};
}

Vec3 operator *(float f, Vec3 v)
{
    return {.x = v.x*f, .y = v.y*f, .z = v.z*f};
}

Vec3 operator /(Vec3 v, float f)
{
    return {.x = v.x/f, .y = v.y/f, .z = v.z/f};
}

Vec4 operator +(Vec4 a, Vec4 b)
{
    return {.x = a.x+b.x, .y = a.y+b.y, .z = a.z+b.z};
}

Vec4 operator -(Vec4 a, Vec4 b)
{
    return {.x = a.x-b.x, .y = a.y-b.y, .z = a.z-b.z};
}

Vec3& operator +=(Vec3& a, Vec3 b)
{
    a = {.x = a.x+b.x, .y = a.y+b.y, .z = a.z+b.z};
    return a;
}

Vec3& operator -=(Vec3& a, Vec3 b)
{
    a = {.x = a.x-b.x, .y = a.y-b.y, .z = a.z-b.z};
    return a;
}

Vec3 operator -(Vec3 v)
{
    return {.x=-v.x, .y=-v.y, .z=-v.z};
}

Vec3& operator *=(Vec3& v, float f)
{
    v = {.x = v.x*f, .y = v.y*f, .z = v.z*f};
    return v;
}

Vec3& operator /=(Vec3& v, float f)
{
    v = {.x = v.x/f, .y = v.y/f, .z = v.z/f};
    return v;
}

Vec3 normalize(Vec3 v)
{
    return v / magnitude(v);
}

float magnitude(Vec3 v)
{
    return sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
}

float dot(Vec3 v1, Vec3 v2)
{
    return v1.x*v2.x + v1.y*v2.y + v1.z*v2.z;
}

Vec3 MoveTowards(Vec3 current, Vec3 target, float delta)
{
    Vec3 diff = target - current;
    float dist = magnitude(diff);
    
    if(dist <= delta) return target;
    return current + diff / dist * delta;
}

Vec2 operator +(Vec2 a, Vec2 b)
{
    return {.x = a.x+b.x, .y = a.y+b.y};
}

Vec2 operator -(Vec2 a, Vec2 b)
{
    return {.x = a.x-b.x, .y = a.y-b.y};
}

Vec2 operator *(Vec2 v, float f)
{
    return {.x = v.x*f, .y = v.y*f};
}

Vec2 operator *(float f, Vec2 v)
{
    return {.x = v.x*f, .y = v.y*f};
}

Vec2 operator /(Vec2 v, float f)
{
    return {.x = v.x/f, .y = v.y/f};
}

Vec2& operator +=(Vec2& a, Vec2 b)
{
    a = {.x = a.x+b.x, .y = a.y+b.y};
    return a;
}

Vec2& operator -=(Vec2& a, Vec2 b)
{
    a = {.x = a.x-b.x, .y = a.y-b.y};
    return a;
}

Vec2 operator -(Vec2 v)
{
    return {.x=-v.x, .y=-v.y};
}

Vec2& operator *=(Vec2& v, float f)
{
    v = {.x = v.x*f, .y = v.y*f};
    return v;
}

Vec2& operator /=(Vec2& v, float f)
{
    v = {.x = v.x/f, .y = v.y/f};
    return v;
}

Quat& operator *=(Quat& a, Quat b)
{
    a.w = a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z;
    a.x = a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y;
    a.y = a.w * b.y + a.y * b.w + a.z * b.x - a.x * b.z;
    a.z = a.w * b.z + a.z * b.w + a.x * b.y - a.y * b.x;
    return a;
}

Quat operator *(Quat a, Quat b)
{
    Quat res;
    res.w = a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z;
    res.x = a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y;
    res.y = a.w * b.y + a.y * b.w + a.z * b.x - a.x * b.z;
    res.z = a.w * b.z + a.z * b.w + a.x * b.y - a.y * b.x;
    return res;
}

// Rotates vector along quaternion
Vec3 operator *(Quat q, Vec3 v)
{
    float num   = q.x * 2.0f;
    float num2  = q.y * 2.0f;
    float num3  = q.z * 2.0f;
    float num4  = q.x * num;
    float num5  = q.y * num2;
    float num6  = q.z * num3;
    float num7  = q.x * num2;
    float num8  = q.x * num3;
    float num9  = q.y * num3;
    float num10 = q.w * num;
    float num11 = q.w * num2;
    float num12 = q.w * num3;
    Vec3 result;
    result.x = (1.0f - (num5 + num6)) * v.x + (num7 - num12) * v.y + (num8 + num11) * v.z;
    result.y = (num7 + num12) * v.x + (1.0f - (num4 + num6)) * v.y + (num9 - num10) * v.z;
    result.z = (num8 - num11) * v.x + (num9 + num10) * v.y + (1.0f - (num4 + num5)) * v.z;
    return result;
}

Quat normalize(Quat q)
{
    float mag = magnitude(q);
    q.x /= mag;
    q.y /= mag;
    q.z /= mag;
    q.w /= mag;
    return q;
}

float magnitude(Quat q)
{
    return sqrt(q.x*q.x + q.y*q.y + q.z*q.z + q.w*q.w);
}

Quat inverse(Quat q)
{
    float lengthSqr = q.x*q.x + q.y*q.y + q.z*q.z + q.w*q.w;
    if(lengthSqr == 0.0f) return q;
    
    float i = 1.0f / lengthSqr;
    return { .w=q.w*i, .x=q.x*-i, .y=q.y*-i, .z=q.z*-i };
}

Quat slerp(Quat q1, Quat q2, float t)
{
    // Check that this works
    TODO;
    
    float lengthSqr1 = q1.x*q1.x + q1.y*q1.y + q1.z*q1.z + q1.w*q1.w;
    float lengthSqr2 = q2.x*q2.x + q2.y*q2.y + q2.z*q2.z + q2.w*q2.w;
    if(lengthSqr1 == 0.0f)
    {
        if(lengthSqr2 == 0.0f)
            return Quat::identity;
        return q2;
    }
    else if(lengthSqr2 == 0.0f)
        return q1;
    
    float cosHalfAngle = q1.w * q2.w + dot(q1.xyz, q2.xyz);
    if(cosHalfAngle >= 1.0f || cosHalfAngle <= -1.0f)
        return q1;
    if(cosHalfAngle < 0.0f)
    {
        q2 = {.w=-q2.w, .xyz=-q2.xyz};
        cosHalfAngle = -cosHalfAngle;
    }
    
    float blendA = 0.0f;
    float blendB = 0.0f;
    if(cosHalfAngle < 0.99f)
    {
        // Do proper slerp for big angles
        float halfAngle = acos(cosHalfAngle);
        float sinHalfAngle = sin(halfAngle);
        float oneOverSinHalfAngle = 1.0f / sinHalfAngle;
        blendA = sin(halfAngle * (1.0f - t)) * oneOverSinHalfAngle;
        blendB = sin(halfAngle * t) * oneOverSinHalfAngle;
    }
    else
    {
        // Do lerp if angle is really small
        blendA = 1.0f - t;
        blendB = t;
    }
    
    Quat result = {.w = blendA*q1.w + blendB*q2.w, .xyz = blendA*q1.xyz + blendB*q2.xyz};
    float lengthSqr = q1.x*q1.x + q1.y*q1.y + q1.z*q1.z + q1.w*q1.w;
    if(lengthSqr > 0.0f)
        return normalize(result);
    
    return Quat::identity;
}

Quat AngleAxis(Vec3 axis, float angle)
{
    if(dot(axis, axis) == 0.0f)
        return Quat::identity;
    
    angle *= 0.5f;
    axis = normalize(axis);
    axis *= sin(angle);
    
    return { .w=(float)cos(angle), .x=axis.x, .y=axis.y, .z=axis.z };
}

float AngleDiff(Quat a, Quat b)
{
    float f = dot(a.xyz, b.xyz) + a.w*b.w;
    return acos(min(abs(f), 1.0f)) * 2.0f;
}

Quat RotateTowards(Quat current, Quat target, float delta)
{
    float angle = AngleDiff(current, target);
    if(angle == 0.0f) return target;
    
    float t = min(1.0f, delta / angle);
    return slerp(current, target, t);
}

Mat4 World2ViewMatrix(Vec3 camPos, Quat camRot)
{
    Vec3& p = camPos;
    Quat r = normalize(inverse(camRot));
    
    // Apply quaternion rotation:
    // 2*(q0^2 + q1^2) - 1    2*(q1*q2 - q0*q3)      2*(q1*q3 + q0*q2)
    // 2*(q1*q2 + q0*q3)      2*(q0^2 + q2^2) - 1    2*(q2*q3 - q0*q1)
    // 2*(q1*q3 - q0*q2)      2*(q2*q3 + q0*q1)      2*(q0^2 + q3^2) - 1
    Mat4 res
    (1 -2*r.y*r.y -2*r.z*r.z, 2*r.x*r.y +2*r.w*r.z,    2*r.x*r.z -2*r.w*r.y,    0,
     2*r.x*r.y -2*r.w*r.z,    1 -2*r.x*r.x -2*r.z*r.z, 2*r.y*r.z +2*r.w*r.x,    0,
     2*r.x*r.z +2*r.w*r.y,    2*r.y*r.z -2*r.w*r.x,    1 -2*r.x*r.x -2*r.y*r.y, 0,
     0,                       0,                       0,                       1);
    
    // Apply inverse of rotation first, then inverse of translation
    res.m14 = res.column1.x * -p.x + res.column2.x * -p.y + res.column3.x * p.z;
    res.m24 = res.column1.y * -p.x + res.column2.y * -p.y + res.column3.y * p.z;
    res.m34 = res.column1.z * -p.x + res.column2.z * -p.y + res.column3.z * p.z;
    
    return res; 
}

Mat4 View2ProjMatrix(float nearClip, float farClip, float fov, float aspectRatio)
{
    const float n = nearClip;
    const float f = farClip;
    const float r = n * tan(fov / 2.0f);
    const float t = r / aspectRatio;
    
    // This is the View->Projection matrix
    Mat4 res
    (n/r, 0,   0,            0,
     0,   n/t, 0,            0,
     0,   0,   -(f+n)/(f-n), -2*f*n/(f-n),
     0,   0,   -1,           1);
    
    return res;
}

// Memory Allocations

inline static uintptr_t AlignForward(uintptr_t ptr, size_t align)
{
    assert(IsPowerOf2(align));
    
    uintptr_t a = (uintptr_t) align;
    
    // This is the same as (ptr % a) but faster
    // since a is known to be a power of 2
    uintptr_t modulo = ptr & (a-1);
    
    // If the address is not aligned, push the address
    // to the next value which is aligned
    if(modulo != 0)
        ptr += a - modulo;
    
    return ptr;
}

void ArenaInit(Arena* arena,
               void* backingBuffer,
               size_t backingBufferLength,
               size_t commitSize)
{
    arena->buffer     = (unsigned char*) backingBuffer;
    arena->length     = backingBufferLength;
    arena->offset     = 0;
    arena->prevOffset = 0;
    arena->commitSize = commitSize;
    
    if(commitSize > 0)
        OS_MemCommit(backingBuffer, commitSize);
}

Arena ArenaVirtualMemInit(size_t reserveSize, size_t commitSize)
{
    assert(commitSize > 0);
    
    Arena result = {0};
    result.buffer     = (unsigned char*)OS_MemReserve(reserveSize);
    result.length     = reserveSize;
    result.offset     = 0;
    result.prevOffset = 0;
    result.commitSize = commitSize;
    
    assert(result.buffer);
    OS_MemCommit(result.buffer, commitSize);
    
    return result;
}

// TODO: Handle exceeding the buffer size
void* ArenaAlloc(Arena* arena, size_t size, size_t align)
{
    uintptr_t curPtr = (uintptr_t) arena->buffer + (uintptr_t) arena->offset;
    uintptr_t offset = AlignForward(curPtr, align);
    
    // Convert to relative offset
    offset -= (uintptr_t) arena->buffer;
    
    // Check to see if the backing memory has space left
    if(offset + size <= arena->length)
    {
        uintptr_t nextOffset = offset + size;
        
        if(arena->commitSize > 0)
        {
            uintptr_t commitAligned = nextOffset - (nextOffset %
                                                    arena->commitSize); 
            if(commitAligned > arena->offset)
            {
                size_t toCommit = commitAligned + arena->commitSize;
                OS_MemCommit(arena->buffer, toCommit);
            }
        }
        
        void* ptr = &arena->buffer[offset];
        arena->offset  = nextOffset;
        arena->prevOffset = offset;
        
        // Zero new memory for debugging
#ifndef NDEBUG
        memset(ptr, 0, size);
#endif
        
        return ptr;
    }
    
    return 0;
}

void* ArenaResizeLastAlloc(Arena* arena, void* oldMemory, size_t oldSize, size_t newSize, size_t align)
{
    unsigned char* oldMem = (unsigned char*)oldMemory;
    assert(IsPowerOf2(align));
    
    if(!oldMem || oldSize == 0)
        return ArenaAlloc(arena, newSize, align);
    else if(arena->buffer <= oldMem && oldMem < (arena->buffer + arena->length))
    {
        if((arena->buffer + arena->prevOffset) == oldMem)
        {
            auto prevOffset = arena->offset;
            arena->offset = arena->prevOffset + newSize;
            if(newSize > oldSize)
            {
                // Commit if needed
                if(arena->commitSize > 0)
                {
                    uintptr_t commitAligned = arena->offset - (arena->offset %
                                                               arena->commitSize); 
                    if(commitAligned > prevOffset)
                    {
                        size_t toCommit = commitAligned + arena->commitSize;
                        OS_MemCommit(arena->buffer, toCommit);
                    }
                }
                
                // Zero new memory for debugging
#ifndef NDEBUG
                memset(&arena->buffer[arena->prevOffset + oldSize], 0, newSize - oldSize);
#endif
            }
            
            return oldMemory;
        }
        else
        {
            void* newMemory = ArenaAlloc(arena, newSize, align);
            size_t copySize = oldSize < newSize ? oldSize : newSize;
            // Copy across old memory to the new memory
            memmove(newMemory, oldMemory, copySize);
            return newMemory;
        }
    }
    
    assert(false && "Arena exceeded memory limit");
    return 0;
}

void* ArenaAllocAndCopy(Arena* arena, void* toCopy, size_t size, size_t align)
{
    void* result = ArenaAlloc(arena, size, align);
    memcpy(result, toCopy, size);
    return result;
}

ArenaTemp ArenaTempBegin(Arena* arena)
{
    ArenaTemp tmp;
    tmp.arena      = arena;
    tmp.offset     = arena->offset;
    tmp.prevOffset = arena->prevOffset;
    return tmp;
}

void ArenaTempEnd(ArenaTemp tmp)
{
    assert(tmp.offset >= 0);
    assert(tmp.prevOffset >= 0);
    tmp.arena->offset     = tmp.offset;
    tmp.arena->prevOffset = tmp.prevOffset;
}

#define NumScratchArenas 4
static thread_local Arena scratchArenas[NumScratchArenas];

// Some procedures might accept arenas as arguments,
// for dynamically allocated results yielded to the caller.
// The argument arena might, in turn, be a scratch arena used
// by the caller for some temporary computation. This means
// that whenever a procedure wants a thread-local scratch buffer
// it needs to provide the arenas that they're already using,
// so that the provided scratch arena does not conflict with anything
//
// For more information on memory arenas: https://www.rfleury.com/p/untangling-lifetimes-the-arena-allocator
ArenaTemp GetScratchArena(Arena** conflictArray, int count)
{
    assert(count < NumScratchArenas-1);  // Should at least be room for one
    Arena* result = 0;
    Arena* scratch = scratchArenas;
    for(int i = 0; i < ArrayCount(scratchArenas); ++i)
    {
        bool conflict = false;
        for(int j = 0; j < count; ++j)
        {
            if(&scratch[i] == conflictArray[j])
            {
                conflict = true;
                break;
            }
        }
        
        if(!conflict)
        {
            result = &scratch[i];
            break;
        }
    }
    
    return ArenaTempBegin(result);
}

ArenaTemp GetScratchArena(int idx)
{
    assert(idx < NumScratchArenas);
    return ArenaTempBegin(&scratchArenas[idx]);
}

ScratchArena::ScratchArena()
{
    this->tempGuard = GetScratchArena(0, 0);
}

ScratchArena::ScratchArena(Arena* a1)
{
    this->tempGuard = GetScratchArena(&a1, 1);
}

ScratchArena::ScratchArena(Arena* a1, Arena* a2)
{
    Arena* conflicts[] = { a1, a2 };
    this->tempGuard = GetScratchArena(conflicts, ArrayCount(conflicts));
}

ScratchArena::ScratchArena(Arena* a1, Arena* a2, Arena* a3)
{
    Arena* conflicts[] = { a1, a2, a3 };
    this->tempGuard = GetScratchArena(conflicts, ArrayCount(conflicts));
}

ScratchArena::ScratchArena(int idx)
{
    this->tempGuard = GetScratchArena(idx);
}