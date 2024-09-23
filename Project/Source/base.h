
#pragma once

#include <utility>
#include <cstdio>
#include <stdlib.h>
#include <cstdint>
#include <cstring>
#include <cstdarg>
#include <string>
#include <cmath>
#include <cassert>
#include <xmmintrin.h>
#include <intrin.h>

#include "os/os_generic.h"

// For small platform dependent utility functions
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#elif defined(__linux__)
#error "Not currently supported"
#elif defined(__APPLE__)
#error "Not currently supported"
#else
#error "Unknown operating system"
#endif

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
#define ArrayCount(array) sizeof(array) / sizeof(array[0])

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

#define StrLit(str) String{.ptr=str, .len=sizeof(str)}
#define StrPrintf(str) (int)str.len, str.ptr
#define ArrToSlice(array) {.ptr=array, .len=ArrayCount(array)}

#define SmallNumber (1.e-8f)

// SIMD Handy macros

#define MakeShuffleMask(x,y,z,w)           (x | (y<<2) | (z<<4) | (w<<6))
// vec(0, 1, 2, 3) -> (vec[x], vec[y], vec[z], vec[w])
#define VecSwizzleMask(vec, mask)          _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(vec), mask))
#define VecSwizzle(vec, x, y, z, w)        VecSwizzleMask(vec, MakeShuffleMask(x,y,z,w))
#define VecSwizzle1(vec, x)                VecSwizzleMask(vec, MakeShuffleMask(x,x,x,x))
// special swizzle
#define VecSwizzle_0022(vec)               _mm_moveldup_ps(vec)
#define VecSwizzle_1133(vec)               _mm_movehdup_ps(vec)

// return (vec1[x], vec1[y], vec2[z], vec2[w])
#define VecShuffle(vec1, vec2, x,y,z,w)    _mm_shuffle_ps(vec1, vec2, MakeShuffleMask(x,y,z,w))
// special shuffle
#define VecShuffle_0101(vec1, vec2)        _mm_movelh_ps(vec1, vec2)
#define VecShuffle_2323(vec1, vec2)        _mm_movehl_ps(vec2, vec1)

////
// Hash functions
u32 Murmur32Seed(void const* data, s64 len, u32 seed);
u64 Murmur64Seed(void const* data, s64 len, u64 seed);
inline u32 Murmur32(void const* data, s64 len) { return Murmur32Seed(data, len, 0x9747b28c); }
inline u64 Murmur64(void const* data, s64 len) { return Murmur64Seed(data, len, 0x9747b28c); }

////
// Simple math functions
#define Pi 3.14159265358979323846f
#define DoublePi 3.14159265358979323846

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
inline int max(int i1, int i2)
{
    return i1 < i2 ? i2 : i1;
}

inline int min(int i1, int i2)
{
    return i1 < i2 ? i1 : i2;
}

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

inline int clamp(int v, int min, int max)
{
    return v < min? min : v > max? max : v;
}

inline float lerp(float a, float b, float t)
{
    return a + (b-a)*t;
}

inline int sign(float v)
{
    return v == 0.0f ? 0 : v > 0.0f ? 1 : -1;
}

inline int sign(int v)
{
    return v == 0 ? 0 : v > 0 ? 1 : -1;
}

inline float ApproachExponential(float source, float target, float smoothing, float deltaTime)
{
    return lerp(source, target, 1.0f - (float)pow(smoothing, deltaTime));
}

inline bool IsPowerOf2(uint32_t n)
{
    return (n & (n-1)) == 0;
}

inline bool IsPowerOf2(uint64_t n)
{
    return (n & (n-1)) == 0;
}

inline uint32_t NextPowerOf2(uint32_t n)
{
    --n;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    ++n;
    return n;
}

inline int32_t NextPowerOf2(int32_t n)
{
    --n;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    ++n;
    return n;
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
    static const Vec3 zero;
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
Vec3 cross(Vec3 v1, Vec3 v2);
Vec3 ApproachLinear(Vec3 current, Vec3 target, float delta);
Vec3 lerp(Vec3 v1, Vec3 v2);

Vec4 lerp(Vec4 v1, Vec4 v2);

// Row major
struct Mat3
{
    union
    {
        struct
        {
            float m11, m12, m13;
            float m21, m22, m23;
            float m31, m32, m33;
        };
        float m[3][3];  // Since it's in row major order, first index is the row and the second one is the column
        Vec3 rows[3];
    };
    
    static const Mat3 identity;
};

// Row major
#pragma pack(16)
struct Mat4
{
    union
    {
        struct
        {
            float m11, m12, m13, m14;
            float m21, m22, m23, m24;
            float m31, m32, m33, m34;
            float m41, m42, m43, m44;
        };
        float m[4][4];  // Since it's in row major order, first index is the row and the second one is the column
        Vec4 rows[4];
        __m128 rowsSimd[4];
    };
    
    static const Mat4 identity;
};

Mat3 ToMat3(const Mat4& mat);

Mat4& operator *=(Mat4& m1, Mat4 m2);
Mat4 operator *(const Mat4& m1, const Mat4& m2);

Mat4 transpose(const Mat4& m);

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
Quat FromToRotation(Vec3 from, Vec3 to);

struct Transform
{
    Vec3 position;
    Quat rotation;
    Vec3 scale;
};

Mat4 RotationMatrix(Quat rot);
Mat4 ScaleMatrix(Vec3 scale);
Mat4 PositionMatrix(Vec3 pos);
Mat4 Mat4FromPosRotScale(Vec3 pos, Quat rot, Vec3 scale);
void PosRotScaleFromMat4(Mat4 mat, Vec3* pos, Quat* rot, Vec3* scale);
Vec3 QuatToEulerRad(Quat q);
Quat EulerRadToQuat(Vec3 euler);
Vec3 EulerRadToDeg(Vec3 euler);
Vec3 EulerDegToRad(Vec3 euler);
Vec3 NormalizeDegAngles(Vec3 euler);
float NormalizeDegAngle(float angle);
Vec3 NormalizeRadAngles(Vec3 angles);
float NormalizeRadAngle(float angle);

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

// This is like a "string view" of sorts. It doesn't actually
// handle allocating the memory and all that. It's pretty much
// a read-only slice of chars.
struct String
{
    const char* ptr;
    // This struct will be 16 bytes anyway (padding), so might as well use them.
    // It's signed because we don't really need the 64th bit, and it's better to prevent underflows
    int64_t len;
    
#ifdef BoundsChecking
    // For reading the value
    inline char operator [](int idx) const { assert(idx < length); return ptr[idx]; };
#else
    // For reading the value
    inline char operator [](int idx) const { return ptr[idx]; };
#endif
};

bool StringBeginsWith(String s, char* beginsWith);
bool StringBeginsWith(String s, String beginsWith);
String RemoveLeadingAndTrailingSpaces(String s);
String ToLenStr(char* str);
String ToLenStr(const char* str);
b32 operator ==(String s1, String s2);
b32 operator !=(String s1, String s2);
b32 operator ==(String s1, const char* s2);
b32 operator !=(String s1, const char* s2);
char* ToCString(String s);      // NOTE: Allocates memory!
wchar_t* ToWCString(String s);  // NOTE: Allocates memory!
void WriteToFile(String s, FILE* file);

////
// Basic data structures
struct Arena;

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

// Dynamically growing array, which uses arenas
// Should be zero initialized
template<typename t>
struct Array
{
    t* ptr = 0;
    int32_t len = 0;
    int32_t capacity = 0;
    Arena* arena = 0;
    
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

// This should actually be its own type with
// functions that null terminate automatically
typedef Array<char> DynString;

template<typename t>
void UseArena(Array<t>* array);
template<typename t>
void Resize(Array<t>* array, int newSize);
template<typename t>
void ResizeExact(Array<t>* array, int newSize);
template<typename t>
void Append(Array<t>* array, t el);
template<typename t>
void Pop(Array<t>* array);
template<typename t>
Slice<t> ToSlice(Array<t>* array);
template<typename t, size_t n>
Slice<t> ToSlice(std::array<t, n>& arr);
template<typename t>
Slice<t> CopyToArena(Array<t>* array, Arena* arena);
template<typename t>
void Free(Array<t>* array);

// Dynamically growing string table.
// This structure does not own the strings,
// those need to be separately allocated
#include <unordered_map>

struct StringHash
{
    std::size_t operator()(const String& s) const
    {
        // Using the simple FNV-1a hash function
        
        std::size_t hash = 0xcbf29ce484222325; // FNV-1a offset basis
        const char* data = s.ptr;
        
        for (int64_t i = 0; i < s.len; ++i)
        {
            hash ^= (std::size_t)data[i];
            hash *= 0x100000001b3; // FNV-1a prime
        }
        
        return hash;
    }
};

struct StringEqual
{
    bool operator()(const String& s1, const String& s2) const
    {
        return s1 == s2;
    }
};

#define StringMapLoadFactor 0.8f
template<typename t>
struct StringMap
{
    Array<char> stringStorage;
    
    struct Cell
    {
        bool occupied;
        String key;
        t value;
    };
    
    Array<Cell> cells;
    int numFilled;
};

template<typename t>
void Append(StringMap<t>* map, String key, const t& value);
template<typename t>
void Append(StringMap<t>* map, const char* key, const t& value);
template<typename t>
bool Lookup(StringMap<t>* map, String key, t* outResult);
template<typename t>
void Free(StringMap<t>* map);

////
// Memory allocation

void* MemReserve(uint64_t size);
void MemCommit(void* mem, uint64_t size);
void MemFree(void* mem, uint64_t size);

#define ArenaDefAlign sizeof(void*)

#define ArenaAllocTyped(type, arenaPtr) (type*)ArenaAlloc(arenaPtr, sizeof(type), alignof(type))
#define ArenaZAllocTyped(type, arenaPtr) (type*)ArenaZAlloc(arenaPtr, sizeof(type), alignof(type))
#define ArenaAllocArray(type, size, arenaPtr) (type*)ArenaAlloc(arenaPtr, sizeof(type)*(size), alignof(type));
#define ArenaZAllocArray(type, size, arenaPtr) (type*)ArenaZAlloc(arenaPtr, sizeof(type)*(size), alignof(type));

struct Arena
{
    unsigned char* buffer;
    size_t length;
    size_t offset;
    size_t prevOffset;
    
    // Commit memory in blocks of size
    // commitSize; if == 0, then it never
    // commits (useful for stack-allocated arenas)
    size_t commitSize;
};

// Can be used like:
// ArenaTemp tempGuard = Arena_TempBegin(arena);
// defer { Arena_TempEnd(tempGuard); };
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
    inline void Reset()    { Arena_TempEnd(tempGuard); };
    inline Arena* arena()    { return tempGuard.arena; };
    inline operator Arena*() { return tempGuard.arena; };
};

uintptr_t AlignForward(uintptr_t ptr, size_t align);

// Initialize the arena with a pre-allocated buffer
void ArenaInit(Arena* arena, void* backingBuffer,
               size_t backingBufferLength, size_t commitSize);
Arena ArenaVirtualMemInit(size_t reserveSize, size_t commitSize);
void* ArenaAlloc(Arena* arena,
                 size_t size, size_t align = ArenaDefAlign);
void* ArenaZAlloc(Arena* arena, size_t size, size_t align = ArenaDefAlign);
void* ArenaResizeLastAlloc(Arena* arena, void* oldMemory,
                           size_t oldSize, size_t newSize,
                           size_t align = ArenaDefAlign);
void* ArenaResizeAndZeroLastAlloc(Arena* arena, void* oldMemory,
                                  size_t oldSize, size_t newSize,
                                  size_t align = ArenaDefAlign);
void* ArenaAllocAndCopy(Arena* arena, void* toCopy,
                        size_t size, size_t align = ArenaDefAlign);

char* ArenaPushNullTermString(Arena* arena, const char* str);
String ArenaPushString(Arena* arena, const char* str);
char* ArenaPushNullTermString(Arena* arena, String str);
String ArenaPushString(Arena* arena, String str);
template<typename t>
Slice<t> ArenaPushSlice(Arena* arena, Slice<t> slice);

void ArenaWriteToFile(Arena* arena, FILE* file);

// Used to free all the memory within the allocator
// by setting the buffer offsets to zero
inline void ArenaFreeAll(Arena* arena)
{
    arena->offset     = 0;
    arena->prevOffset = 0;
}

inline void ArenaReleaseMem(Arena* arena)
{
    TODO;
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
// String construction utils

// Can zero initialize this. This is something that should
// not be put in a tightly packed structure in memory, because
// API ergonomics are prioritized over struct size. (The operations
// themselves are still performant though)
struct StringBuilder
{
    Array<char> str;
    Arena* arena;  // If nullptr, just use malloc and free
};

inline void UseArena(StringBuilder* builder, Arena* arena);
void Append(StringBuilder* builder, const char* str);
void Append(StringBuilder* builder, char c);
void Append(StringBuilder* builder, String str);
void AppendFmt(StringBuilder* builder, const char* fmt, ...);
void NullTerminate(StringBuilder* builder);
// Insert binary representation of value in string
// (Useful for serialization)
// NOTE: inserts padding as necessary
template<typename t>
void Put(StringBuilder* builder, t val);
void FreeBuffers(StringBuilder* builder);
String ToString(StringBuilder* builder);
// Consume stream of bytes
template<typename t>
t Next(const char** cursor);
template<typename t>
t Next(const char** cursor, int count);
String Next(const char** cursor, int strLen);

////
// Filesystem utils
String LoadEntireFile(const char* path, Arena* dst, bool* outSuccess);
char* LoadEntireFileAndNullTerminate(const char* path, Arena* dst, bool* outSuccess);
String LoadEntireFile(String path, Arena* dst, bool* outSuccess);
char* LoadEntireFileAndNullTerminate(String path, Arena* dst, bool* outSuccess);

String GetPathExtension(const char* path);
String GetPathExtension(String path);
String GetPathNoExtension(const char* path);
String GetPathNoExtension(String path);
String PopLastDirFromPath(const char* path);
String PopLastDirFromPath(String path);

char* GetExecutablePath();
bool B_SetCurrentDirectory(const char* path);
bool SetCurrentDirectoryRelativeToExe(const char* path);
char* B_GetCurrentDirectory(Arena* dst);
String GetFullPath(const char* path, Arena* dst);

////
// Simple Text file handling
struct TextFileHandler
{
    String file;
    char* at;
    bool ok;
    int lineNum;
};

struct TextLine
{
    int num;
    String text;
    bool ok;
};

struct TwoStrings
{
    String a;
    String b;
};

TextFileHandler LoadTextFile(String path, Arena* dst);
TextLine ConsumeNextLine(TextFileHandler* handler);
TwoStrings BreakByChar(TextLine line, char c);
void ParseValue(String* string);

////
// Miscellaneous

// Defer statement (similar to that of Go, Jai and Odin)
// Usage:
#if 0
int _test()
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

void B_Sleep(uint64_t millis);
void DebugMessage(const char* message);
void DebugMessageFmt(const char* fmt, ...);
bool B_IsDebuggerPresent();
