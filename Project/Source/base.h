
// Things I could add
// 1) I could add some nice to use string utilities
//    (at least as nice to use as std::string, but
//     without the unnecessary allocations)

#pragma once

#include <utility>
#include <stdio.h>
#include <stdlib.h>
#include <cstdint>
#include <cstdint>
#include <cstring>
#include "os/os_generic.h"

// Handy typedefs
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef uint32_t b32;
typedef int8_t   s8;
typedef int16_t  s16;
typedef int32_t  s32;
typedef int64_t  s64;
typedef float    f32;
typedef double   f64;
typedef unsigned char uchar;

// Handy macros
#define TODO assert(!"This is not implemented!")

#define KB(num) ((num)*1024LLU)
#define MB(num) (KB(num)*1024LLU)
#define GB(num) (MB(num)*1024LLU)

#if defined(_WIN64) || defined(__x86_64__) || defined(_M_X64) || defined(__64BIT__) || defined(__powerpc64__) || defined(__ppc64__)
#ifndef Bit64
#define Bit64 1
#endif
#else
#ifndef Bit32
#define Bit32 1
#endif
#endif

#define ToLenStr(str) String{.ptr=str, .len=((s64)strlen(str))}
#define StrLit(str) String{.ptr=str, .len=sizeof(str)}

////
// Hash functions
u32 Murmur32Seed(void const* data, s64 len, u32 seed);
u64 Murmur64Seed(void const* data, s64 len, u64 seed);
inline u32 Murmur32(void const* data, s64 len) { return Murmur32Seed(data, len, 0x9747b28c); }
inline u64 Murmur64(void const* data, s64 len) { return Murmur64Seed(data, len, 0x9747b28c); }

////
// Simple math functions
#define Pi 3.14159265358979323846

inline float sqr(float f)
{
    return f*f;
}

inline double sqr(double d)
{
    return d*d;
}

#ifndef min
inline float min(float f1, float f2)
{
    return f1 > f2 ? f2 : f1;
}

inline double min(double f1, double f2)
{
    return f1 > f2 ? f2 : f1;
}
#endif

#ifndef max
inline float max(float f1, float f2)
{
    return f1 < f2 ? f2 : f1;
}

inline double max(double f1, double f2)
{
    return f1 < f2 ? f2 : f1;
}
#endif

inline float clamp(float v, float min, float max)
{
    return v < min? min : v > max? max : v;
}

inline bool IsPowerOf2(uint32_t n)
{
    return (n & (n-1)) == 0;
}

inline bool IsPowerOf2(uint64_t n)
{
    return (n & (n-1)) == 0;
}

inline float Deg2Rad(float deg)
{
    return deg * Pi / 180;
}

inline float Rad2Deg(float rad)
{
    return rad * 180 / Pi;
}

////
// Vectors and matrices
struct Vec3
{
    float x, y, z;
    
    static const Vec3 right;
    static const Vec3 up;
    static const Vec3 forward;
    static const Vec3 left;
    static const Vec3 down;
    static const Vec3 backward;
};

struct Vec4
{
    float x, y, z, w;
};

struct Vec2
{
    float x, y;
};

Vec3 operator +(Vec3 a, Vec3 b);
Vec3& operator +=(Vec3& a, Vec3 b);
Vec3 operator -(Vec3 a, Vec3 b);
Vec3& operator -=(Vec3& a, Vec3 b);
Vec3 operator -(Vec3 v);
Vec3 operator *(Vec3 v, float f);
Vec3 operator *(float f, Vec3 v);
Vec3& operator *=(Vec3& v, float f);
Vec3 operator /(Vec3 v, float f);
Vec3& operator /=(Vec3& v, float f);

Vec2 operator +(Vec2 a, Vec2 b);
Vec2& operator +=(Vec2& a, Vec2 b);
Vec2 operator -(Vec2 a, Vec2 b);
Vec2& operator -=(Vec2& a, Vec2 b);
Vec2 operator -(Vec2 v);
Vec2 operator *(Vec2 v, float f);
Vec2 operator *(float f, Vec2 v);
Vec2& operator *=(Vec2& v, float f);
Vec2 operator /(Vec2 v, float f);
Vec2& operator /=(Vec2& v, float f);

Vec4 operator +(Vec4 a, Vec4 b);
Vec4& operator +=(Vec4& a, Vec4 b);
Vec4 operator -(Vec4 a, Vec4 b);
Vec4& operator -=(Vec4& a, Vec4 b);
Vec4 operator -(Vec4 v);
Vec4 operator *(Vec4 v, float f);
Vec4 operator *(float f, Vec4 v);
Vec4& operator *=(Vec4& v, float f);
Vec4 operator /(Vec4 v, float f);
Vec4& operator /=(Vec4& v, float f);

Vec3 normalize(Vec3 v);
float magnitude(Vec3 v);
float dot(Vec3 v1, Vec3 v2);
Vec3 MoveTowards(Vec3 current, Vec3 target, float delta);

// Column major
struct Mat4
{
    union
    {
        struct
        {
            float m11, m21, m31, m41;
            float m12, m22, m32, m42;
            float m13, m23, m33, m43;
            float m14, m24, m34, m44;
        };
        struct
        {
            Vec4 c1, c2, c3, c4;
        };
    };
    
    // This "constructor" allows to specify the elements in the
    // normal math notation (which would be in row major order
    // with C's struct initializer). It's not an actual constructor
    // because if it were it wouldn't let me initialize with an
    // aggregate initialization list (for some reason).
    inline void set(float a11, float a12, float a13, float a14,
                    float a21, float a22, float a23, float a24,
                    float a31, float a32, float a33, float a34,
                    float a41, float a42, float a43, float a44)
    {
        m11 = a11; m12 = a12; m13 = a13; m14 = a14;
        m21 = a21, m22 = a22, m23 = a23, m24 = a24;
        m31 = a31, m32 = a32, m33 = a33, m34 = a34;
        m41 = a41, m42 = a42, m43 = a43, m44 = a44;
    }
    
    static const Mat4 identity;
};

struct Quat
{
    float w;
    union
    {
        struct
        {
            float x, y, z;
        };
        Vec3 xyz;
    };
    
    static const Quat identity;
};

Quat& operator *=(Quat& a, Quat b);
Quat operator *(Quat a, Quat b);

Quat normalize(Quat q);
float magnitude(Quat q);
Quat inverse(Quat q);
Quat AngleAxis(Vec3 axis, float angle);
Quat RotateTowards(Quat current, Quat target, float delta);

struct Transform
{
    Vec3 position;
    Quat rotation;
    Vec3 scale;
};

Mat4 World2ViewMatrix(Vec3 camPos, Quat camRot);
Mat4 View2ProjMatrix(float nearClip, float farClip, float fov, float aspectRatio); 

////
// Utility macros for linked lists.
// The reason these are macros is because templates
// generate lots of dumb functions, and you can't really
// use intrusive linked-lists, you have to have a struct
// exclusively with the purpose of holding a pointer and
// some state (which is not really relevant to the list part)
// DLL = Doubly Linked List, SLL = Singly Linked List

#if 0
#define DLLPushBack(f, l, n) (((f)==0? \
(f)=(l)=(n):\
(l)->next=(n),(l)=(n)),\
(n)->next=(n)->prev=0) \

//#define DLLPushFront(f, l, n) ...
//#define DLLRemove(
#endif

////
// Length strings utils

struct String
{
    char* ptr;
    int64_t len;
};

////
// Memory allocation

#define ArenaDefAlign sizeof(void*)

#define ArenaAllocType(type, arenaPtr) (type*)ArenaAlloc(arenaPtr, sizeof(type), alignof(type))
#define ArenaAllocArray(type, size, arenaPtr) (type*)ArenaAlloc(arenaPtr, sizeof(type)*(size), alignof(type));

struct Arena
{
    unsigned char* buffer;
    size_t length;
    size_t offset;
    size_t prevOffset;
    
    // Commit memory in blocks of size
    // commitSize, if == 0, then it never
    // commits (useful for stack-allocated arenas)
    size_t commitSize;
};

// Can be used like:
// ArenaTemp tempGaurd = Arena_TempBegin(arena);
// defer(Arena_TempEnd(tempGuard);
struct ArenaTemp
{
    Arena* arena;
    size_t offset;
    size_t prevOffset;
};

inline ArenaTemp Arena_TempBegin(Arena* arena)
{
    ArenaTemp tmp;
    tmp.arena      = arena;
    tmp.offset     = arena->offset;
    tmp.prevOffset = arena->prevOffset;
    return tmp;
}

inline void Arena_TempEnd(ArenaTemp tmp)
{
    assert(tmp.offset >= 0);
    assert(tmp.prevOffset >= 0);
    tmp.arena->offset     = tmp.offset;
    tmp.arena->prevOffset = tmp.prevOffset;
}

// Serves as a helper for obtaining a scratch
// arena. It also resets the arena's state upon
// destruction.
struct ScratchArena
{
    ArenaTemp tempGuard;
    
    // NOTE(Leo): In case a ScratchArena is passed to the
    // constructor with the intent to do a user implemented cast to
    // Arena*, the actual result is the copy constructor is called,
    // which is not expected. This makes an error appear instead of
    // causing a (silent) bug.
    ScratchArena(ScratchArena& scratch) = delete;
    
    // This could be templatized, but...
    // we're not going to need more than
    // 4 scratch arenas anyway, and you could
    // always add more constructors
    inline ScratchArena();
    inline ScratchArena(Arena* a1);
    inline ScratchArena(Arena* a1, Arena* a2);
    inline ScratchArena(Arena* a1, Arena* a2, Arena* a3);
    
    // This constructor is used to get a certain scratch arena,
    // without handling potential conflicts. It can be used when
    // using multiple scratch arenas in a single function which
    // doesn't allocate anything to be used by the caller (so,
    // it doesn't accept an Arena* as parameter)
    inline ScratchArena(int idx);
    
    inline ~ScratchArena() { Arena_TempEnd(tempGuard); };
    inline void Reset() { Arena_TempEnd(tempGuard); };
    inline Arena* arena() { return tempGuard.arena; };
    inline operator Arena*() { return tempGuard.arena; };
};

uintptr_t AlignForward(uintptr_t ptr, size_t align);

// Initialize the arena with a pre-allocated buffer
void ArenaInit(Arena* arena, void* backingBuffer,
               size_t backingBufferLength, size_t commitSize);
Arena ArenaVirtualMemInit(size_t reserveSize, size_t commitSize);
void* ArenaAlloc(Arena* arena,
                 size_t size, size_t align = ArenaDefAlign);
void* ArenaResizeLastAlloc(Arena* arena, void* oldMemory,
                           size_t oldSize, size_t newSize,
                           size_t align = ArenaDefAlign);
void* ArenaAllocAndCopy(Arena* arena, void* toCopy,
                        size_t size, size_t align = ArenaDefAlign);

template<typename t>
t* ArenaPushVar(Arena* arena, t var);
template<typename t>
t ReadAlignedNextValue(uintptr_t* ptr);
char* ArenaPushString(Arena* arena, const char* str);
char* ArenaPushStringNoNullTerm(Arena* arena, const char* str);

void ArenaWriteToFile(Arena* arena, FILE* file);

// Used to free all the memory within the allocator
// by setting the buffer offsets to zero
inline void ArenaFreeAll(Arena* arena)
{
    arena->offset     = 0;
    arena->prevOffset = 0;
}

ArenaTemp ArenaTempBegin(Arena* arena);
void ArenaTempEnd(ArenaTemp tmp);
ArenaTemp GetScratchArena(Arena** conflicts, uint64_t conflictCount);  // Grabs a thread-local scratch arena
ArenaTemp GetScratchArena(int idx);

#define ReleaseScratch(scratch) ArenaTempEnd(scratch);

////
// Error handling

enum LogKind
{
    Log_Error = 0,
    Log_Warning,
    Log_Message,
};

struct LogError
{
    LogKind kind;
    char* log;
    // This is wasted space but simplifies the API,
    // instead of the Arena* and LogError* you only
    // need to pass a LogError and it will contain the
    // appropriate arena.
    Arena* owner;
    LogError* next;
};

////
// Basic data structures
template<typename t>
struct Slice
{
    t* ptr;
    int64_t len;
    
#ifdef BoundsChecking
    // For reading the value
    inline t  operator [](int idx) const { assert(idx < length); return ptr[idx]; };
    // For writing to the value (this returns a left-value)
    inline t& operator [](int idx) { assert(idx < length); return ptr[idx]; };
#else
    // For reading the value
    inline t  operator [](int idx) const { return ptr[idx]; };
    // For writing to the value (this returns a left-value)
    inline t& operator [](int idx) { return ptr[idx]; };
#endif
};

////
// Miscellaneous

#define ArrayCount(array) sizeof(array) / sizeof(array[0])

// Defer statement (similar to that of Go, Jai and Odin)
// Usage:
#if 0
int test()
{
    int* array = (int*)malloc(sizeof(int)*4);
    defer { free(array) };  // This will be executed at the end of the scope
    
    // ... other code
}
#endif
template<typename t>
struct Defer
{
    Defer(t var) : var(var) {}
    ~Defer() { var(); }
    t var;
};

template<typename t>
Defer<t> deferCreate(t var) { return Defer<t>(var); }

#define defer__(line) defer_ ## line
#define defer_(line) defer__(line)

struct DeferDummy {};

template<typename t>
Defer<t> operator +(DeferDummy, t&& var) { return deferCreate<t>(std::forward<t>(var)); }

#define defer auto defer_(__LINE__) = DeferDummy() + [&]()
