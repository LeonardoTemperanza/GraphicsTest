
// NOTE: @incomplete Things that do not work right now:
// Incomplete Matrix stuff
// List operations with macros are not implemented (maybe they're not needed in this project)
// String utilities could be made for ease of use of std::string but without the extra allocations and deallocations
// OS path utilities

#include "base.h"

////
// Hash functions
// From Bill Hall's "gb.h" helper library
// https://github.com/gingerBill/gb/blob/master/gb.h
u32 Murmur32Seed(void const* data, s64 len, u32 seed)
{
    u32 const c1 = 0xcc9e2d51;
    u32 const c2 = 0x1b873593;
    u32 const r1 = 15;
    u32 const r2 = 13;
    u32 const m  = 5;
    u32 const n  = 0xe6546b64;
    
    s64 i, nblocks = len / 4;
    u32 hash = seed, k1 = 0;
    u32 const *blocks = (u32 const*)data;
    u8 const *tail = (u8 const *)(data) + nblocks*4;
    
    for (i = 0; i < nblocks; i++) {
        u32 k = blocks[i];
        k *= c1;
        k = (k << r1) | (k >> (32 - r1));
        k *= c2;
        
        hash ^= k;
        hash = ((hash << r2) | (hash >> (32 - r2))) * m + n;
    }
    
    switch (len & 3) {
        case 3:
        k1 ^= tail[2] << 16;
        case 2:
        k1 ^= tail[1] << 8;
        case 1:
        k1 ^= tail[0];
        
        k1 *= c1;
        k1 = (k1 << r1) | (k1 >> (32 - r1));
        k1 *= c2;
        hash ^= k1;
    }
    
    hash ^= len;
    hash ^= (hash >> 16);
    hash *= 0x85ebca6b;
    hash ^= (hash >> 13);
    hash *= 0xc2b2ae35;
    hash ^= (hash >> 16);
    
    return hash;
}

// From Bill Hall's "gb.h" helper library 
// https://github.com/gingerBill/gb/blob/master/gb.h
u64 Murmur64Seed(void const* data_, s64 len, u64 seed)
{
#if defined(Bit64)
    u64 const m = 0xc6a4a7935bd1e995ULL;
    s32 const r = 47;
    
    u64 h = seed ^ (len * m);
    
    u64 const *data = (u64 const *)data_;
    u8  const *data2 = (u8 const *)data_;
    u64 const* end = data + (len / 8);
    
    while (data != end) {
        u64 k = *data++;
        
        k *= m;
        k ^= k >> r;
        k *= m;
        
        h ^= k;
        h *= m;
    }
    
    switch (len & 7) {
        case 7: h ^= (u64)(data2[6]) << 48;
        case 6: h ^= (u64)(data2[5]) << 40;
        case 5: h ^= (u64)(data2[4]) << 32;
        case 4: h ^= (u64)(data2[3]) << 24;
        case 3: h ^= (u64)(data2[2]) << 16;
        case 2: h ^= (u64)(data2[1]) << 8;
        case 1: h ^= (u64)(data2[0]);
        h *= m;
    };
    
    h ^= h >> r;
    h *= m;
    h ^= h >> r;
    
    return h;
#else
    u64 h;
    u32 const m = 0x5bd1e995;
    s32 const r = 24;
    
    u32 h1 = (u32)(seed) ^ (u32)(len);
    u32 h2 = (u32)(seed >> 32);
    
    u32 const *data = (u32 const *)data_;
    
    while (len >= 8) {
        u32 k1, k2;
        k1 = *data++;
        k1 *= m;
        k1 ^= k1 >> r;
        k1 *= m;
        h1 *= m;
        h1 ^= k1;
        len -= 4;
        
        k2 = *data++;
        k2 *= m;
        k2 ^= k2 >> r;
        k2 *= m;
        h2 *= m;
        h2 ^= k2;
        len -= 4;
    }
    
    if (len >= 4) {
        u32 k1 = *data++;
        k1 *= m;
        k1 ^= k1 >> r;
        k1 *= m;
        h1 *= m;
        h1 ^= k1;
        len -= 4;
    }
    
    switch (len) {
        case 3: h2 ^= ((u8 const *)data)[2] << 16;
        case 2: h2 ^= ((u8 const *)data)[1] <<  8;
        case 1: h2 ^= ((u8 const *)data)[0] <<  0;
        h2 *= m;
    };
    
    h1 ^= h2 >> 18;
    h1 *= m;
    h2 ^= h1 >> 22;
    h2 *= m;
    h1 ^= h2 >> 17;
    h1 *= m;
    h2 ^= h1 >> 19;
    h2 *= m;
    
    h = h1;
    h = (h << 32) | h2;
    
    return h;
#endif
}

const Vec3 Vec3::right    = { 1.0f,  0.0f,  0.0f};
const Vec3 Vec3::up       = { 0.0f,  1.0f,  0.0f};
const Vec3 Vec3::forward  = { 0.0f,  0.0f,  1.0f};
const Vec3 Vec3::left     = {-1.0f,  0.0f,  0.0f};
const Vec3 Vec3::down     = { 0.0f, -1.0f,  0.0f};
const Vec3 Vec3::backward = { 0.0f,  0.0f, -1.0f};
const Vec3 Vec3::zero     = { 0.0f,  0.0f,  0.0f};

const Quat Quat::identity = {.w=1.0f, .x=0.0f, .y=0.0f, .z=0.0f};

const Mat3 Mat3::identity =
{
    1, 0, 0,
    0, 1, 0,
    0, 0, 1
};

const Mat4 Mat4::identity =
{ 
    1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 1, 0,
    0, 0, 0, 1
};

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

Vec3 cross(Vec3 v1, Vec3 v2)
{
    return {.x=v1.y*v2.z - v1.z*v2.y, .y=v1.z*v2.x - v1.x*v2.z, .z=v1.x*v2.y - v1.y*v2.x};
}

Vec3 ApproachLinear(Vec3 current, Vec3 target, float delta)
{
    Vec3 diff = target - current;
    float dist = magnitude(diff);
    
    if(dist <= delta) return target;
    return current + diff / dist * delta;
}

Vec3 lerp(Vec3 v1, Vec3 v2, float t)
{
    return v1 + (v2 - v1) * t;
}

Vec4 lerp(Vec4 v1, Vec4 v2, float t)
{
    return v1 + (v2 - v1) * t;
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

Vec4 operator +(Vec4 a, Vec4 b)
{
    return {.x = a.x+b.x, .y = a.y+b.y, .z = a.z+b.z, .w = a.w+b.w};
}

Vec4 operator -(Vec4 a, Vec4 b)
{
    return {.x = a.x-b.x, .y = a.y-b.y, .z = a.z-b.z, .w = a.w-b.w};
}

Vec4 operator *(Vec4 v, float f)
{
    return {.x = v.x*f, .y = v.y*f, .z = v.z*f, .w = v.w*f};
}

Vec4 operator *(float f, Vec4 v)
{
    return {.x = v.x*f, .y = v.y*f, .z = v.z*f, .w = v.w*f};
}

Vec4 operator /(Vec4 v, float f)
{
    return {.x = v.x/f, .y = v.y/f, .z = v.z/f, .w = v.w/f};
}

Vec4& operator +=(Vec4& a, Vec4 b)
{
    a = {.x = a.x+b.x, .y = a.y+b.y, .z = a.z+b.z, .w = a.w+b.w};
    return a;
}

Vec4& operator -=(Vec4& a, Vec4 b)
{
    a = {.x = a.x-b.x, .y = a.y-b.y, .z = a.z-b.z, .w = a.w-b.w};
    return a;
}

Vec4 operator -(Vec4 v)
{
    return {.x=-v.x, .y=-v.y, .z=-v.z, .w=-v.w};
}

Vec4& operator *=(Vec4& v, float f)
{
    v = {.x = v.x*f, .y = v.y*f, .z = v.z*f, .w = v.w*f};
    return v;
}

Vec4& operator /=(Vec4& v, float f)
{
    v = {.x = v.x/f, .y = v.y/f, .z = v.z/f, .w = v.w/f};
    return v;
}

Mat3 ToMat3(const Mat4& mat)
{
    Mat3 res;
    // @speed
    for(int i = 0; i < 3; ++i)
    {
        for(int j = 0; j < 3; ++j)
            res.m[i][j] = mat.m[i][j];
    }
    
    return res;
}

// @speed
Mat4& operator *=(Mat4& m1, Mat4 m2)
{
    m1 = m1 * m2;
    return m1;
}

Mat4 operator *(const Mat4& m1, const Mat4& m2)
{
    Mat4 res;
    for(int i = 0; i < 4; ++i)
    {
        for(int j = 0; j < 4; ++j)
        {
            res.m[i][j] = 0.0f;
            
            for(int k = 0; k < 4; ++k)
                res.m[i][j] += m1.m[i][k] * m2.m[k][j];
        }
    }
    
    return res;
}

Mat4 transpose(const Mat4& m)
{
    Mat4 res
    {
        m.m11, m.m21, m.m31, m.m41,
        m.m12, m.m22, m.m32, m.m42,
        m.m13, m.m23, m.m33, m.m43,
        m.m14, m.m24, m.m34, m.m44
    };
    return res;
}

float Determinant(const Mat4& m) {
    return
        m.m11 * (m.m22 * (m.m33 * m.m44 - m.m34 * m.m43) - m.m23 * (m.m32 * m.m44 - m.m34 * m.m42) + m.m24 * (m.m32 * m.m43 - m.m33 * m.m42)) -
        m.m12 * (m.m21 * (m.m33 * m.m44 - m.m34 * m.m43) - m.m23 * (m.m31 * m.m44 - m.m34 * m.m41) + m.m24 * (m.m31 * m.m43 - m.m33 * m.m41)) +
        m.m13 * (m.m21 * (m.m32 * m.m44 - m.m34 * m.m42) - m.m22 * (m.m31 * m.m44 - m.m34 * m.m41) + m.m24 * (m.m31 * m.m42 - m.m32 * m.m41)) -
        m.m14 * (m.m21 * (m.m32 * m.m43 - m.m33 * m.m42) - m.m22 * (m.m31 * m.m43 - m.m33 * m.m41) + m.m23 * (m.m31 * m.m42 - m.m32 * m.m41));
}

// Requires this matrix to be transform matrix
inline Mat4 ComputeTransformInverse(const Mat4& inM)
{
	Mat4 r;
    
	// transpose 3x3, we know m03 = m13 = m23 = 0
	__m128 t0 = VecShuffle_0101(inM.rowsSimd[0], inM.rowsSimd[1]); // 00, 01, 10, 11
	__m128 t1 = VecShuffle_2323(inM.rowsSimd[0], inM.rowsSimd[1]); // 02, 03, 12, 13
	r.rowsSimd[0] = VecShuffle(t0, inM.rowsSimd[2], 0,2,0,3); // 00, 10, 20, 23(=0)
	r.rowsSimd[1] = VecShuffle(t0, inM.rowsSimd[2], 1,3,1,3); // 01, 11, 21, 23(=0)
	r.rowsSimd[2] = VecShuffle(t1, inM.rowsSimd[2], 0,2,2,3); // 02, 12, 22, 23(=0)
    
	// (SizeSqr(rowsSimd[0]), SizeSqr(rowsSimd[1]), SizeSqr(rowsSimd[2]), 0)
	__m128 sizeSqr;
	sizeSqr =                     _mm_mul_ps(r.rowsSimd[0], r.rowsSimd[0]);
	sizeSqr = _mm_add_ps(sizeSqr, _mm_mul_ps(r.rowsSimd[1], r.rowsSimd[1]));
	sizeSqr = _mm_add_ps(sizeSqr, _mm_mul_ps(r.rowsSimd[2], r.rowsSimd[2]));
    
	// optional test to avoid divide by 0
	__m128 one = _mm_set1_ps(1.f);
	// for each component, if(sizeSqr < SmallNumber) sizeSqr = 1;
	__m128 rSizeSqr = _mm_blendv_ps(
                                    _mm_div_ps(one, sizeSqr),
                                    one,
                                    _mm_cmplt_ps(sizeSqr, _mm_set1_ps(SmallNumber))
                                    );
    
	r.rowsSimd[0] = _mm_mul_ps(r.rowsSimd[0], rSizeSqr);
	r.rowsSimd[1] = _mm_mul_ps(r.rowsSimd[1], rSizeSqr);
	r.rowsSimd[2] = _mm_mul_ps(r.rowsSimd[2], rSizeSqr);
    
	// last line
	r.rowsSimd[3] =                       _mm_mul_ps(r.rowsSimd[0], VecSwizzle1(inM.rowsSimd[3], 0));
	r.rowsSimd[3] = _mm_add_ps(r.rowsSimd[3], _mm_mul_ps(r.rowsSimd[1], VecSwizzle1(inM.rowsSimd[3], 1)));
	r.rowsSimd[3] = _mm_add_ps(r.rowsSimd[3], _mm_mul_ps(r.rowsSimd[2], VecSwizzle1(inM.rowsSimd[3], 2)));
	r.rowsSimd[3] = _mm_sub_ps(_mm_setr_ps(0.f, 0.f, 0.f, 1.f), r.rowsSimd[3]);
    
	return r;
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

// https://stackoverflow.com/questions/46156903/how-to-lerp-between-two-quaternions
Quat lerp(Quat q1, Quat q2, float t)
{
    float dotQ = q1.x * q2.x + q1.y * q2.y + q1.z * q2.z + q1.w * q2.w;
    if(dotQ < 0.0f)
    {
        // Negate q2
        q2.x = -q2.x;
        q2.y = -q2.y;
        q2.z = -q2.z;
        q2.w = -q2.w;
    }
    
    // res = q1 + t(q2 - q1)  -->  res = q1 - t(q1 - q2)
    // The latter is slightly better on x64
    Quat res;
    res.x = q1.x - t*(q1.x - q2.x);
    res.y = q1.y - t*(q1.y - q2.y);
    res.z = q1.z - t*(q1.z - q2.z);
    res.w = q1.w - t*(q1.w - q2.w);
    return normalize(res);
}

// Angle in radians
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

Quat FromToRotation(Vec3 from, Vec3 to)
{
    if(std::abs(dot(from, to)) > 0.999f)
        return Quat::identity;
    
    Quat q;
    Vec3 a = cross(from, to);
    q.xyz = a;
    q.w = std::sqrt(dot(from, from) * dot(to, to)) + dot(from, to);
    return q;
}

Mat4 RotationMatrix(Quat q)
{
    float x = q.x * 2.0f; float y = q.y * 2.0f; float z = q.z * 2.0f;
    float xx = q.x * x;   float yy = q.y * y;   float zz = q.z * z;
    float xy = q.x * y;   float xz = q.x * z;   float yz = q.y * z;
    float wx = q.w * x;   float wy = q.w * y;   float wz = q.w * z;
    
    // Calculate 3x3 matrix from orthonormal basis
    Mat4 res
    {
        1.0f - (yy + zz), xy - wz,          xz + wy,          0.0f,
        xy + wz,          1.0f - (xx + zz), yz - wx,          0.0f,
        xz - wy,          yz + wx,          1.0f - (xx + yy), 0.0f,
        0.0f,             0.0f,             0.0f,             1.0f
    };
    return res;
}

Mat4 ScaleMatrix(Vec3 scale)
{
    Mat4 res
    {
        scale.x, 0.0f,    0.0f,    0.0f,
        0.0f,    scale.y, 0.0f,    0.0f,
        0.0f,    0.0f,    scale.z, 0.0f,
        0.0f,    0.0f,    0.0f,    1.0f
    };
    return res;
}

Mat4 TranslationMatrix(Vec3 pos)
{
    Mat4 res
    {
        1.0f, 0.0f, 0.0f, pos.x,
        0.0f, 1.0f, 0.0f, pos.y,
        0.0f, 0.0f, 1.0f, pos.z,
        0.0f, 0.0f, 0.0f, 1.0f
    };
    return res;
}

Mat4 Mat4FromPosRotScale(Vec3 pos, Quat rot, Vec3 scale)
{
    return TranslationMatrix(pos) * RotationMatrix(normalize(rot)) * ScaleMatrix(scale);
}

void PosRotScaleFromMat4(Mat4 m, Vec3* pos, Quat* rot, Vec3* scale)
{
    scale->x = sqrt(m.m11 * m.m11 + m.m12 * m.m12 + m.m13 * m.m13);
    scale->y = sqrt(m.m21 * m.m21 + m.m22 * m.m22 + m.m23 * m.m23);
    scale->z = sqrt(m.m31 * m.m31 + m.m32 * m.m32 + m.m33 * m.m33);
    
    pos->x = m.m14;
    pos->y = m.m24;
    pos->z = m.m34;
    
    Quat q;
    float m00 = m.m11 / scale->x;
    float m01 = m.m12 / scale->x;
    float m02 = m.m13 / scale->x;
    float m10 = m.m21 / scale->y;
    float m11 = m.m22 / scale->y;
    float m12 = m.m23 / scale->y;
    float m20 = m.m31 / scale->z;
    float m21 = m.m32 / scale->z;
    float m22 = m.m33 / scale->z;
    
    float trace = m00 + m11 + m22;
    if (trace > 0) {
        float s = 0.5f / sqrt(trace + 1.0f);
        q.w = 0.25f / s;
        q.x = (m21 - m12) * s;
        q.y = (m02 - m20) * s;
        q.z = (m10 - m01) * s;
    } else {
        if (m00 > m11 && m00 > m22) {
            float s = 2.0f * sqrt(1.0f + m00 - m11 - m22);
            q.w = (m21 - m12) / s;
            q.x = 0.25f * s;
            q.y = (m01 + m10) / s;
            q.z = (m02 + m20) / s;
        } else if (m11 > m22) {
            float s = 2.0f * sqrt(1.0f + m11 - m00 - m22);
            q.w = (m02 - m20) / s;
            q.x = (m01 + m10) / s;
            q.y = 0.25f * s;
            q.z = (m12 + m21) / s;
        } else {
            float s = 2.0f * sqrt(1.0f + m22 - m00 - m11);
            q.w = (m10 - m01) / s;
            q.x = (m02 + m20) / s;
            q.y = (m12 + m21) / s;
            q.z = 0.25f * s;
        }
    }
    
    *rot = q;
}

float SafeAtan2(double y, double x)
{
    if(x == 0.0f) return 0.0f;
    return (float)std::atan2(y, x);
}

Vec3 QuatToEulerRad(Quat q)
{
    Vec3 angles;
    
    // Roll (X-axis rotation)
    double sinr_cosp = 2.0 * (q.w * q.x + q.y * q.z);
    double cosr_cosp = 1.0 - 2.0 * (q.x * q.x + q.y * q.y);
    angles.x = (float)std::atan2(sinr_cosp, cosr_cosp);
    
    // Pitch (Y-axis rotation)
    double sinp = 2.0 * (q.w * q.y - q.z * q.x);
    if (std::abs(sinp) >= 1.0)
        angles.y = (float)std::copysign(Pi / 2.0, sinp); // use ±90 degrees if out of range
    else
        angles.y = (float)std::asin(sinp);
    
    // Yaw (Z-axis rotation)
    double siny_cosp = 2.0 * (q.w * q.z + q.x * q.y);
    double cosy_cosp = 1.0 - 2.0 * (q.y * q.y + q.z * q.z);
    angles.z = (float)std::atan2(siny_cosp, cosy_cosp);
    
    return angles;
}

Quat EulerRadToQuat(Vec3 euler)
{
    return normalize(AngleAxis(Vec3::up, euler.x) * AngleAxis(Vec3::right, euler.y) * AngleAxis(Vec3::forward, euler.z));
}

Vec3 EulerRadToDeg(Vec3 euler)
{
    return {.x=Rad2Deg(euler.x), .y=Rad2Deg(euler.y), .z=Rad2Deg(euler.z)};
}

Vec3 EulerDegToRad(Vec3 euler)
{
    return {.x=Deg2Rad(euler.x), .y=Deg2Rad(euler.y), .z=Deg2Rad(euler.z)};
}

Vec3 NormalizeDegAngles(Vec3 euler)
{
    return {.x=NormalizeDegAngle(euler.x), .y=NormalizeDegAngle(euler.y), .z=NormalizeDegAngle(euler.z)};
}

Vec3 NormalizeRadAngles(Vec3 euler)
{
    return {.x=NormalizeRadAngle(euler.x), .y=NormalizeRadAngle(euler.y), .z=NormalizeRadAngle(euler.z)};
}

float NormalizeDegAngle(float angle)
{
    // TODO @speed
    
    while(angle < 0.0f) angle += 360.0f;
    while(angle >= 360.0f) angle -= 360.0f;
    return angle;
}

float NormalizeRadAngle(float angle)
{
    // TODO @speed
    
    while(angle < 2.0f * Pi) angle += 2.0f * Pi;
    while(angle >= 2.0f * Pi) angle -= 2.0f * Pi;
    return angle;
}

Quat EulerToQuat(Vec3 eulerDegrees)
{
    Vec3 euler;
    euler.x = eulerDegrees.x * Pi / 180.0f;
    euler.y = eulerDegrees.y * Pi / 180.0f;
    euler.z = eulerDegrees.z * Pi / 180.0f;
    
    float cy = cos(euler.y * 0.5f);
    float sy = sin(euler.y * 0.5f);
    float cp = cos(euler.x * 0.5f);
    float sp = sin(euler.x * 0.5f);
    float cr = cos(euler.z * 0.5f);
    float sr = sin(euler.z * 0.5f);
    
    Quat q;
    q.w = cr * cp * cy + sr * sp * sy;
    q.x = sr * cp * cy - cr * sp * sy;
    q.y = cr * sp * cy + sr * cp * sy;
    q.z = cr * cp * sy - sr * sp * cy;
    
    return q;
}

Mat4 World2ViewMatrix(Vec3 camPos, Quat camRot)
{
    // Inverse rotation matrix
    //Mat4 res = Mat4::identity;
    Mat4 res = RotationMatrix(normalize(inverse(camRot)));
    // Apply inverse of translation before having applied inverse of rotation
    res = res * TranslationMatrix(-camPos);
    return res;
}

Mat4 View2ProjPerspectiveMatrix(float nearClip, float farClip, float horizontalDegFov, float aspectRatio)
{
    const float n = nearClip;
    const float f = farClip;
    const float r = n * tan(Deg2Rad(horizontalDegFov) / 2.0f);
    const float l = -r;
    const float t = r / aspectRatio;
    const float b = -t;
    
    Mat4 res
    {
        2*n/(r-l), 0,         -(r+l)/(r-l),  0,
        0,         2*n/(t-b), -(t+b)/(t-b),  0,
        0,         0,         +(f+n)/(f-n), -2*f*n/(f-n),
        0,         0,         +1,           0
    };
    
    return res;
}

Mat4 View2ProjOrthographicMatrix(float nearClip, float farClip, float aspectRatio)
{
    TODO;
    return {};
}

////
// String utils
bool StringBeginsWith(String s, const char* beginsWith)
{
    int i;
    for(i = 0; beginsWith[i] != '\0' && i < s.len; ++i)
    {
        if(s[i] != beginsWith[i])
            return false;
    }
    
    if(beginsWith[i] != '\0') return false;
    
    return true;
}

bool StringBeginsWith(String s, String beginsWith)
{
    if(s.len != beginsWith.len) return false;
    
    for(int i = 0; i < s.len; ++i)
    {
        if(s[i] != beginsWith[i])
            return false;
    }
    
    return true;
}

String RemoveLeadingAndTrailingSpaces(String s)
{
    int start = 0;
    while(start < s.len && (s[start] == ' ' || s[start] == '\t' || s[start] == '\r'))
        ++start;
    
    int end = (int)s.len - 1;
    while(end > 0 && (s[end] == ' ' || s[end] == '\t' || s[end] == '\r'))
        --end;
    
    if(end - start <= 0) return {0};
    
    String res = {};
    res.ptr = s.ptr + start;
    res.len = end - start + 1;
    return res;
}

String ToLenStr(char* str)
{
    return {.ptr=str, .len=(int64_t)strlen(str)};
}

String ToLenStr(const char* str)
{
    return {.ptr=str, .len=(int64_t)strlen(str)};
}

b32 operator ==(String s1, String s2)
{
    if(s1.len != s2.len) return false;
    
    for(int i = 0; i < s1.len; ++i)
    {
        if(s1.ptr[i] != s2.ptr[i]) return false;
    }
    
    return true;
}

b32 operator !=(String s1, String s2)
{
    return !(s1 == s2);
}

b32 operator ==(String s1, const char* s2)
{
    int i;
    for(i = 0; i < s1.len && s2[i]; ++i)
    {
        if(s1.ptr[i] != s2[i]) return false;
    }
    
    if(i < s1.len || s2[i]) return false;
    
    return true;
}

b32 operator !=(String s1, const char* s2)
{
    return !(s1 == s2);
}

// NOTE: Allocates memory!
char* ToCString(String s)
{
    char* res = (char*)malloc(s.len+1);
    memcpy(res, s.ptr, s.len);
    res[s.len] = '\0';
    return res;
}

// NOTE: Allocates memory!
wchar_t* ToWCString(String s)
{
    wchar_t* res = (wchar_t*)malloc(s.len * sizeof(wchar_t)+1);
    for(int i = 0; i < s.len; ++i)
        res[i] = (wchar_t) s.ptr[i];
    
    res[s.len] = '\0';
    return res;
}

void WriteToFile(String s, FILE* file)
{
    assert(file);
    fwrite(s.ptr, s.len, 1, file);
}

////
// String construction utils

inline void UseArena(StringBuilder* builder, Arena* arena)
{
    assert(builder->str.len == 0);
    builder->arena = arena;
}

inline void Append(StringBuilder* builder, const char* str)
{
    int64_t len = (int64_t)strlen(str);
    String lenStr = {.ptr=str, .len=len};
    Append(builder, lenStr);
}

inline void Append(StringBuilder* builder, char c)
{
    int64_t len = 1;
    String str = {.ptr=&c, .len=len};
    Append(builder, str);
}

void Append(StringBuilder* builder, String str)
{
    int32_t oldLen = builder->str.len;
    int32_t newLen = oldLen + (int32_t)str.len;
    int32_t oldCapacity = builder->str.capacity;
    
    if(newLen > builder->str.capacity)
        builder->str.capacity = NextPowerOf2(max(oldCapacity + 1, newLen) + 1);
    
    builder->str.len = newLen;
    
    // NOTE: The alignment needs to be 8 here, for consistent binary encoding/decoding
    char* newPtr = nullptr;
    if(builder->arena)
        newPtr = (char*)ArenaResizeLastAlloc(builder->arena, builder->str.ptr, oldCapacity, builder->str.capacity, 8);
    else
        newPtr = (char*)_aligned_realloc(builder->str.ptr, builder->str.capacity, 8);
    
    builder->str.ptr = newPtr;
    memcpy(builder->str.ptr + oldLen, str.ptr, str.len);
}

void AppendFmt(StringBuilder* builder, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    
    // vasprintf is not widely supported right now, so doing
    // this instead. TODO: Might just want to implement my own printf
    // since it's so limited.
    int len = vsnprintf(nullptr, 0, fmt, args);
    int oldLen = builder->str.len;
    int newLen = oldLen + len;
    int oldCapacity = builder->str.capacity;
    
    // Do not call Append to avoid extra allocation
    if(newLen > builder->str.capacity)
        builder->str.capacity = NextPowerOf2(newLen);
    
    builder->str.len = newLen;
    
    // NOTE: The alignment needs to be 8 here, for consistent binary encoding/decoding
    char* newPtr = nullptr;
    if(builder->arena)
        newPtr = (char*)ArenaResizeLastAlloc(builder->arena, builder->str.ptr, oldCapacity, builder->str.capacity, 8);
    else
        newPtr = (char*)_aligned_realloc(builder->str.ptr, builder->str.capacity, 8);
    
    builder->str.ptr = newPtr;
    
    // Insert expanded string into the newly allocated string
    vsnprintf(builder->str.ptr + oldLen, newLen-oldLen, fmt, args);
    
    va_end(args);
}

template<typename t>
void Put(StringBuilder* builder, t val)
{
    uintptr_t offset = AlignForward((uintptr_t)(builder->str.ptr + builder->str.len), alignof(t)) - (uintptr_t)(void*)builder->str.ptr;
    int oldLen = builder->str.len;
    int newLen = (int)offset + sizeof(t);
    int oldCapacity = builder->str.capacity;
    
    // TODO Duplicated code here
    if(newLen > builder->str.capacity)
        builder->str.capacity = NextPowerOf2(newLen);
    
    builder->str.len = newLen;
    
    // NOTE: The alignment needs to be 8 here, for consistent binary encoding/decoding
    char* newPtr = nullptr;
    if(builder->arena)
        newPtr = (char*)ArenaResizeLastAlloc(builder->arena, builder->str.ptr, oldCapacity, builder->str.capacity, 8);
    else
        newPtr = (char*)_aligned_realloc(builder->str.ptr, builder->str.capacity, 8);
    
    builder->str.ptr = newPtr;
    
    // Write the value in the address
    memset(&builder->str[oldLen], 0, newLen-oldLen);
    t* addr = (t*)&builder->str[(int)offset];
    *addr = val;
}

template<typename t>
void PutSlice(StringBuilder* builder, Slice<t> slice)
{
    uintptr_t offset = AlignForward((uintptr_t)(builder->str.ptr + builder->str.len), alignof(t)) - (uintptr_t)(void*)builder->str.ptr;
    int oldLen = builder->str.len;
    int newLen = (int)offset + sizeof(t) * slice.len;
    int oldCapacity = builder->str.capacity;
    
    // TODO Duplicated code here
    if(newLen > builder->str.capacity)
        builder->str.capacity = NextPowerOf2(newLen);
    
    builder->str.len = newLen;
    
    // NOTE: The alignment needs to be 8 here, for consistent binary encoding/decoding
    char* newPtr = nullptr;
    if(builder->arena)
        newPtr = (char*)ArenaResizeLastAlloc(builder->arena, builder->str.ptr, oldCapacity, builder->str.capacity, 8);
    else
        newPtr = (char*)_aligned_realloc(builder->str.ptr, builder->str.capacity, 8);
    
    builder->str.ptr = newPtr;
    
    // Write the value in the address
    memset(&builder->str[oldLen], 0, newLen-oldLen);
    t* addr = (t*)&builder->str[(int)offset];
    memcpy(addr, slice.ptr, sizeof(t) * slice.len);
}

void NullTerminate(StringBuilder* builder)
{
    Put(builder, '\0');
}

void FreeBuffers(StringBuilder* builder)
{
    void* old = (void*)builder->str.ptr;
    builder->str.ptr = nullptr;
    builder->str.len = 0;
    
    // If using an arena, its user will free the contents
    // of the arena when necessary.
    if(!builder->arena)
    {
        // NOTE: We used _aligned_realloc instead of simple realloc,
        // so we need to call _aligned_free to properly free
        _aligned_free(old);
    }
}

inline String ToString(StringBuilder* builder)
{
    return {.ptr=builder->str.ptr, .len=builder->str.len};
}

// Consume stream of bytes (generated using StringBuilder likely)
template<typename t>
t Next(char** cursor)
{
    t* next = (t*)(void*)AlignForward((uintptr_t)(void*)*cursor, alignof(t));
    *cursor = (char*)(next + 1);
    return *next;
}

template<typename t>
Slice<t> Next(char** cursor, int count)
{
    t* next = (t*)(void*)AlignForward((uintptr_t)(void*)*cursor, alignof(t));
    *cursor = (char*)(next + count);
    return {.ptr=next, .len=count};
}

String Next(char** cursor, int strLen)
{
    String res = {.ptr=*cursor, .len=strLen};
    *cursor += strLen;
    return res;
}

////
// Basic data structures
template<typename t>
void UseArena(Array<t>* array, Arena* arena)
{
    array->arena = arena;
}

template<typename t>
void Resize(Array<t>* array, int newSize)
{
    int oldLen = array->len;
    int newLen = newSize;
    int oldCapacity = array->capacity;
    
    if(newLen > array->capacity)
        array->capacity = NextPowerOf2(max(oldCapacity + 1, newLen) + 1);
    
    array->len = newLen;
    
    t* newPtr = nullptr;
    if(array->arena)
        newPtr = (t*)ArenaResizeLastAlloc(array->arena, array->ptr, oldCapacity*sizeof(t), array->capacity*sizeof(t), alignof(t));
    else
        newPtr = (t*)realloc(array->ptr, array->capacity*sizeof(t));
    
    array->ptr = newPtr;
}

template<typename t>
void ResizeExact(Array<t>* array, int newSize)
{
    array->len = newSize;
    int oldCapacity = array->capacity;
    array->capacity = newSize;
    
    t* newPtr = nullptr;
    if(array->arena)
        newPtr = (t*)ArenaResizeLastAlloc(array->arena, array->ptr, oldCapacity, array->capacity, alignof(t));
    else
        newPtr = (t*)realloc(array->ptr, array->capacity*sizeof(t));
    
    array->ptr = newPtr;
}

template<typename t>
void Append(Array<t>* array, t el)
{
    int oldLen = array->len;
    int newLen = oldLen + 1;
    int oldCapacity = array->capacity;
    
    if(newLen > array->capacity)
        array->capacity = NextPowerOf2(max(oldCapacity + 1, newLen) + 1);
    
    array->len = newLen;
    
    t* newPtr = nullptr;
    if(array->arena)
        newPtr = (t*)ArenaResizeLastAlloc(array->arena, array->ptr, oldCapacity*sizeof(t), array->capacity*sizeof(t), alignof(t));
    else
        newPtr = (t*)realloc(array->ptr, array->capacity*sizeof(t));
    
    array->ptr = newPtr;
    array->ptr[array->len - 1] = el;
}

template<typename t>
void Pop(Array<t>* array)
{
    // TODO: Free memory if necessary
    --array->len;
}

template<typename t>
Slice<t> ToSlice(Array<t>* array)
{
    return {.ptr=array->ptr, .len=array->len};
}

template<typename t, size_t n>
Slice<t> ToSlice(std::array<t, n>& array)
{
    return Slice<t>{array.data(), static_cast<int>(n)};
}

template<typename t>
Slice<t> CopyToArena(Array<t>* array, Arena* arena)
{
    return ArenaPushSlice(arena, ToSlice(array));
}

template<typename t>
void Free(Array<t>* array)
{
    if(!array->arena) free(array->ptr);
    
    array->ptr = 0;
    array->len = 0;
    array->capacity = 0;
}

// This is SipHash. Maybe there's a better hash function?
u64 SipHash(String str, u64 seed)
{
    size_t hash = seed;
    for(int i = 0; i < str.len; ++i)
        hash = RotateLeft(hash, 9) + (unsigned char) str[i];
    
    // Thomas Wang 64-to-32 bit mix function, hopefully also works in 32 bits
    hash ^= seed;
    hash = (~hash) + (hash << 18);
    hash ^= hash ^ RotateRight(hash,31);
    hash = hash * 21;
    hash ^= hash ^ RotateRight(hash,11);
    hash += (hash << 6);
    hash ^= RotateRight(hash,22);
    return hash+seed;
}

static u64 mapSizes[] =
{
    31, 67, 131, 257, 521, 1031,
    2053, 4099, 8192, 16411, 32771,
    65617,
};

template<typename t>
void Append(StringMap<t>* map, String key, const t& value)
{
    if(key.len <= 0) return;
    
    if(map->slots.len <= 0)  // Not initialized
    {
        // Allocate the minimum size if empty
        map->slots.ptr = (StringMapSlot<t>*)calloc(mapSizes[0], sizeof(StringMapSlot<t>));
        map->slots.len = mapSizes[0];
        
        // Initialize the arena
        map->stringArena = ArenaVirtualMemInit(GB(4), MB(2));
    }
    
    u64 hash = SipHash(key);
    u64 idx = hash % map->slots.len;
    
    StringMapSlot<t>* slot = &map->slots[idx];
    int probeIndex = 1;
    while(slot->occupied && slot->key != key)
    {
        // Quadratic probing
        idx = (hash + probeIndex * probeIndex) % map->slots.len;
        slot = &map->slots[idx];
        ++probeIndex;
    }
    
    bool found = slot->occupied;
    if(found)
    {
        // If a slot already exists with this key,
        // simply overwrite its value
        map->slots[idx].value = value;
    }
    else
    {
        ++map->numOccupied;
        float loadFactor = ((float)map->numOccupied / map->slots.len);
        if(loadFactor >= StringMapMaxLoadFactor)
        {
            // Allocate bigger buffer
            StringMap<t> newMap = *map;
            ++newMap.sizeIdx;
            assert(newMap.sizeIdx < ArrayCount(mapSizes));
            u64 newSize = mapSizes[newMap.sizeIdx];
            newMap.slots.ptr = (StringMapSlot<t>*)calloc(newSize, sizeof(StringMapSlot<t>));
            newMap.slots.len = newSize;
            
            // Rehash all slots
            for(int i = 0; i < map->slots.len; ++i)
            {
                auto& slot = map->slots[i];
                if(slot.occupied)
                    Append(&newMap, slot.key, slot.value);
            }
            
            free(map->slots.ptr);
            *map = newMap;
        }
        else
        {
            // Allocate string into string storage
            String newString = ArenaPushString(&map->stringArena, key);
            map->slots[idx].occupied = true;
            map->slots[idx].key = newString;
            map->slots[idx].value = value;
        }
    }
}

template<typename t>
void Append(StringMap<t>* map, const char* key, const t& value)
{
    Append(map, ToLenStr(key), value);
}

template<typename t>
LookupResult<t> Lookup(StringMap<t>* map, String key)
{
    u64 idx = LookupIdx(map, key);
    // Not found
    if(idx == -1) return {.res={}, .found = false};
    
    return {.res=&map->slots[idx].value, .found = true};
}

template<typename t>
LookupResult<t> Lookup(StringMap<t>* map, const char* key)
{
    return Lookup(map, ToLenStr(key));
}

template<typename t>
u64 LookupIdx(StringMap<t>* map, String key)
{
    if(key.len <= 0 || map->slots.len <= 0) return -1;
    
    u64 hash = SipHash(key);
    u64 idx = hash % map->slots.len;
    
    StringMapSlot<t>* slot = &map->slots[idx];
    int probeIndex = 1;
    while(slot->occupied && slot->key != key)
    {
        // Quadratic probing
        idx = (hash + probeIndex * probeIndex) % map->slots.len;
        slot = &map->slots[idx];
        ++probeIndex;
    }
    
    bool found = slot->occupied;
    if(!found) return -1;
    
    return idx;
}

template<typename k, typename v>
v*Append(HashMap<k, v>* map, k key, const v& value)
{
    // TODO TODO TODO @speed
    // This is just O(n) instead of O(1)
    
    int found = -1;
    for(int i = 0; i < map->keys.len; ++i)
    {
        if(memcmp(&key, &map->keys[i], sizeof(k)) == 0)
        {
            found = i;
            break;
        }
    }
    
    if(found == -1)  // Not found
    {
        Append(&map->keys, key);
        Append(&map->values, value);
        return &map->values[map->values.len - 1];
    }
    else
    {
        map->values[found] = value;
        return &map->values[found];
    }
}

template<typename k, typename v>
LookupResult<v> Lookup(HashMap<k, v>* map, k key)
{
    // TODO TODO TODO @speed
    // This is just O(n) instead of O(1)
    
    int found = -1;
    for(int i = 0; i < map->keys.len; ++i)
    {
        if(memcmp(&key, &map->keys[i], sizeof(k)) == 0)
        {
            found = i;
            break;
        }
    }
    
    if(found == -1) return {{}, false};
    return {&map->values[found], true};
}

template<typename k, typename v>
void Remove(HashMap<k, v>* map, k key)
{
    int found = -1;
    for(int i = 0; i < map->keys.len; ++i)
    {
        if(memcmp(&key, &map->keys[i], sizeof(k)) == 0)
        {
            found = i;
            break;
        }
    }
    
    if(found != -1)  // Found something
    {
        auto lastIdx = map->keys.len - 1;
        map->keys[lastIdx] = map->keys[found];
        map->values[lastIdx] = map->values[found];
        Pop(&map->keys);
        Pop(&map->values);
    }
}

template<typename k, typename v>
void Free(HashMap<k, v>* map)
{
    Free(&map->keys);
    Free(&map->values);
}

template<typename t>
void Free(StringMap<t>* map)
{
    ArenaFreeAll(&map->stringArena);
    ArenaReleaseMem(&map->stringArena);
    free(map->slots.ptr);
}

#ifdef _WIN32
void* MemReserve(uint64_t size)
{
    void* res = VirtualAlloc(0, size, MEM_RESERVE, PAGE_READWRITE);
    assert(res && "VirtualAlloc failed.");
    if(!res) abort();
    
    return res;
}
#else
#error "Unimplemented for this OS!"
#endif

#ifdef _WIN32
void MemCommit(void* mem, uint64_t size)
{
    void* res = VirtualAlloc(mem, size, MEM_COMMIT, PAGE_READWRITE);
    assert(res && "VirtualAlloc failed.");
    if(!res) abort();
}
#else
#error "Unimplemented for this OS!"
#endif

#ifdef _WIN32
void MemFree(void* mem, uint64_t size)
{
    bool ok = VirtualFree(mem, 0, MEM_RELEASE);
    assert(ok && "VirtualFree failed.");
    if(!ok) abort();
}
#else
#error "Unimplemented for this OS!"
#endif

////
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
        MemCommit(backingBuffer, commitSize);
}

Arena ArenaVirtualMemInit(size_t reserveSize, size_t commitSize)
{
    assert(commitSize > 0);
    
    Arena result = {0};
    result.buffer     = (unsigned char*)MemReserve(reserveSize);
    result.length     = reserveSize;
    result.offset     = 0;
    result.prevOffset = 0;
    result.commitSize = commitSize;
    
    assert(result.buffer);
    MemCommit(result.buffer, commitSize);
    
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
                MemCommit(arena->buffer, toCommit);
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
    
    return nullptr;
}

void* ArenaZAlloc(Arena* arena, size_t size, size_t align)
{
    void* ptr = ArenaAlloc(arena, size, align);
    memset(ptr, 0, size);
    return ptr;
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
                        MemCommit(arena->buffer, toCommit);
                    }
                }
                
                memset(&arena->buffer[arena->prevOffset + oldSize], 0, newSize - oldSize);
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
    
    assert(!"Arena exceeded memory limit");
    return nullptr;
}

void* ArenaResizeAndZeroLastAlloc(Arena* arena, void* oldMemory, size_t oldSize, size_t newSize, size_t align)
{
    void* ptr = ArenaResizeLastAlloc(arena, oldMemory, oldSize, newSize, align);
    memset(ptr, 0, newSize);
    return ptr;
}

void* ArenaAllocAndCopy(Arena* arena, void* toCopy, size_t size, size_t align)
{
    void* result = ArenaAlloc(arena, size, align);
    memcpy(result, toCopy, size);
    return result;
}

char* ArenaPushNullTermString(Arena* arena, const char* str)
{
    int len = (int)strlen(str);
    char* ptr = (char*)ArenaAlloc(arena, len+1, 1);
    ptr[len] = '\0';
    memcpy(ptr, str, len);
    return ptr;
}

String ArenaPushString(Arena* arena, String str)
{
    char* ptr = (char*)ArenaAlloc(arena, str.len, 1);
    memcpy(ptr, str.ptr, str.len);
    return {.ptr=ptr, .len=str.len};
}

String ArenaPushString(Arena* arena, std::string str)
{
    char* ptr = (char*)ArenaAlloc(arena, str.size(), 1);
    memcpy(ptr, str.c_str(), str.size());
    return {.ptr=ptr, .len=(s64)str.size()};
}

char* ArenaPushNullTermString(Arena* arena, String str)
{
    char* ptr = (char*)ArenaAlloc(arena, str.len+1, 1);
    ptr[str.len] = '\0';
    memcpy(ptr, str.ptr, str.len);
    return ptr;
}

String ArenaPushString(Arena* arena, const char* str)
{
    int len = (int)strlen(str);
    char* ptr = (char*)ArenaAlloc(arena, len, 1);
    memcpy(ptr, str, len);
    return {.ptr=ptr, .len=len};
}

template<typename t>
Slice<t> ArenaPushSlice(Arena* arena, Slice<t> slice)
{
    t* ptr = (t*)ArenaAllocArray(t, slice.len, arena);
    memcpy(ptr, slice.ptr, slice.len * sizeof(t));
    return {.ptr=ptr, .len=slice.len};
}

void ArenaWriteToFile(Arena* arena, FILE* file)
{
    assert(file);
    fwrite(arena->buffer, arena->offset, 1, file);
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

// NOTE: Remember to keep all scratch arenas initialized at
// the start of the program.
#define NumScratchArenas 4
static thread_local Arena scratchArenas[NumScratchArenas] = {};

void InitScratchArenas()
{
    for(int i = 0; i < NumScratchArenas; ++i)
        scratchArenas[i] = ArenaVirtualMemInit(GB(4), MB(2));
}

// NOTE: Remember to keep the permanent arena initialized at
// the start of the program.
static Arena permArena = {};

void InitPermArena()
{
    permArena = ArenaVirtualMemInit(GB(4), MB(2));
}

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

////
// Miscellaneous

String LoadEntireFile(const char* path, Arena* dst, bool* outSuccess)
{
    *outSuccess = true;
    
    String res = {0};
    
    // This would make fopen fail
    if(!path || *path == '\0')
    {
        *outSuccess = false;
        res.ptr = nullptr;
        res.len = 0;
        return res;
    }
    
    FILE* file = fopen(path, "rb");
    if(!file)
    {
        *outSuccess = false;
        res.ptr = nullptr;  // This can be 'freed' with no repercussions
        res.len = 0;
        return res;
    }
    defer { fclose(file); };
    
    fseek(file, 0, SEEK_END);
    res.len = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    // NOTE: We're aligning to 8 bytes because it's a common ground
    // when parsing binary files
    size_t align = 8;
    res.ptr = (char*)ArenaAlloc(dst, res.len, align);
    fread((void*)res.ptr, res.len, 1, file);
    
    return res;
}

char* LoadEntireFileAndNullTerminate(const char* path, Arena* dst, bool* outSuccess)
{
    *outSuccess = true;
    
    char* res;
    FILE* file = fopen(path, "rb");
    if(!file)
    {
        *outSuccess = false;
        res = (char*)ArenaAlloc(dst, 1, 1);
        res[0] = '\0';
        return res;
    }
    defer { fclose(file); };
    
    fseek(file, 0, SEEK_END);
    s64 len = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    res = (char*)ArenaAlloc(dst, len+1, 1);
    fread((void*)res, len, 1, file);
    res[len] = '\0';
    
    return res;
}

String LoadEntireFile(String path, Arena* dst, bool* outSuccess)
{
    ScratchArena scratch(dst);
    StringBuilder builder = {};
    UseArena(&builder, scratch);
    
    Append(&builder, path);
    NullTerminate(&builder);
    return LoadEntireFile(ToString(&builder).ptr, dst, outSuccess);
}

char* LoadEntireFileAndNullTerminate(String path, Arena* dst, bool* outSuccess)
{
    ScratchArena scratch(dst);
    StringBuilder builder = {};
    UseArena(&builder, scratch);
    
    Append(&builder, path);
    NullTerminate(&builder);
    return LoadEntireFileAndNullTerminate(ToString(&builder).ptr, dst, outSuccess);
}

String GetPathExtension(const char* path)
{
    int len = (int)strlen(path);
    int lastDotIdx = len-1;
    for(int i = len-1; i >= 0; --i)
    {
        if(path[i] == '.')
        {
            lastDotIdx = i;
            break;
        }
    }
    
    return {.ptr=&path[lastDotIdx+1], .len=len-(lastDotIdx+1)};
}

String GetPathExtension(String path)
{
    int len = (int)path.len;
    int lastDotIdx = len-1;
    for(int i = len-1; i >= 0; --i)
    {
        if(path[i] == '.')
        {
            lastDotIdx = i;
            break;
        }
    }
    
    return {.ptr=path.ptr + lastDotIdx + 1, .len=len-(lastDotIdx+1)};
}

String GetPathNoExtension(const char* path)
{
    int len = (int)strlen(path);
    int lastDotIdx = len;
    for(int i = len-1; i >= 0; --i)
    {
        if(path[i] == '.')
        {
            lastDotIdx = i;
            break;
        }
    }
    
    return {.ptr=path, .len=lastDotIdx};
}

String GetPathNoExtension(String path)
{
    int len = (int)path.len;
    int lastDotIdx = len;
    for(int i = len-1; i >= 0; --i)
    {
        if(path[i] == '.')
        {
            lastDotIdx = i;
            break;
        }
    }
    
    return {.ptr=path.ptr, .len=lastDotIdx};
}

String PopLastDirFromPath(const char* path)
{
    int lastSlashPos = -1;
    int i;
    for(i = 0; path[i] != '\0'; ++i)
    {
        if(path[i] == '/' || path[i] == '\\')
        {
            lastSlashPos = i;
        }
    }
    
    int pathLen = i - 1;
    
    if(lastSlashPos == -1)
        return {.ptr=path, .len=pathLen};
    
    return {.ptr=path, .len=lastSlashPos};
}

String PopLastDirFromPath(String path)
{
    int lastSlashPos = -1;
    int i;
    for(i = 0; i < path.len; ++i)
    {
        if(path[i] == '/' || path[i] == '\\')
        {
            lastSlashPos = i;
        }
    }
    
    if(lastSlashPos == -1)
        return path;
    
    return {.ptr=path.ptr, .len=lastSlashPos};
}

#ifdef _WIN32
char* GetExecutablePath()
{
    char* res = (char*)malloc(MAX_PATH);
    GetModuleFileName(nullptr, res, MAX_PATH);
    return res;
}
#else
#error "Unimplemented for this OS!"
#endif

#ifdef _WIN32
bool B_SetCurrentDirectory(const char* path)
{
    return SetCurrentDirectory(path);
}
#else
#error "Unimplemented for this OS!"
#endif

bool SetCurrentDirectoryRelativeToExe(const char* path)
{
    String exePath = PopLastDirFromPath(GetExecutablePath());
    
    StringBuilder builder = {0};
    defer { FreeBuffers(&builder); };
    
    Append(&builder, exePath);
    
    if(path[0] != '/')
        Append(&builder, "/");
    
    Append(&builder, path);
    NullTerminate(&builder);
    
    String str = ToString(&builder);
    
    bool ok = B_SetCurrentDirectory(str.ptr);
    return ok;
}

#ifdef _WIN32
char* B_GetCurrentDirectory(Arena* dst)
{
    char* res = (char*)ArenaAlloc(dst, MAX_PATH, 1);
    GetCurrentDirectory(MAX_PATH, res);
    return res;
}
#else
#error "Unimplemented for this OS!"
#endif

String GetFullPath(const char* path, Arena* dst)
{
    ScratchArena scratch;
    StringBuilder builder = {0};
    UseArena(&builder, dst);
    
    Append(&builder, B_GetCurrentDirectory(scratch));
    Append(&builder, "/");
    Append(&builder, path);
    return ToString(&builder);
}

TextFileHandler LoadTextFile(String path, Arena* dst)
{
    TextFileHandler handler = {};
    handler.file.ptr = LoadEntireFileAndNullTerminate(path, dst, &handler.ok);
    handler.file.len = strlen(handler.file.ptr);
    if(!handler.ok) return handler;
    
    handler.at = (char*)handler.file.ptr;
    return handler;
}

TextLine ConsumeNextLine(TextFileHandler* handler)
{
    TextLine line = {};
    
    // Eat all whitespace
    while(true)
    {
        if(handler->at[0] == '\n')
        {
            ++handler->at;
            ++handler->lineNum;
        }
        else if(handler->at[0] == ' ' || handler->at[0] == '\t' || handler->at[0] == '\r')
        {
            ++handler->at;
        }
        else if(handler->at[0] == '#')
        {
            ++handler->at;  // Eat '#'
            
            while(handler->at[0] != '\0' && handler->at[0] != '\n')
                ++handler->at;
            
            if(handler->at[0] == '\0')
            {
                line.ok = false;
                return line;
            }
            else  // Currently at a newline
            {
                ++handler->at;
                ++handler->lineNum;
            }
        }
        else
            break;
    }
    
    line.text.ptr = handler->at;
    
    // Reached end before actually getting to the next line
    if(handler->at[0] == '\0')
    {
        line.ok = false;
        return line;
    }
    
    while(handler->at[0] != '\0' && handler->at[0] != '\n')
    {
        ++handler->at;
        ++line.text.len;
    }
    
    line.text = RemoveLeadingAndTrailingSpaces(line.text);
    line.ok = true;
    return line;
}

TextLine GetNextLine(TextFileHandler* handler)
{
    // @cleanup We repeat the same work a second time
    // If we call GetNextLine and then ConsumeNextLine
    TextFileHandler prevState = *handler;
    auto res = ConsumeNextLine(handler);
    *handler = prevState;
    return res;
}

TwoStrings BreakByChar(TextLine line, char c)
{
    TwoStrings strings = {};
    if(!line.ok) return strings;
    
    int firstOccurrence = -1;
    for(int i = 0; i < line.text.len; ++i)
    {
        if(line.text[i] == c)
        {
            firstOccurrence = i;
            break;
        }
    }
    
    if(firstOccurrence == -1)
    {
        strings.a = line.text;
        strings.b = {};
        return strings;
    }
    
    strings.a.ptr = line.text.ptr;
    strings.a.len = firstOccurrence;
    strings.b.ptr = strings.a.ptr + strings.a.len + 1;
    strings.b.len = line.text.len - strings.a.len - 1;
    
    strings.a = RemoveLeadingAndTrailingSpaces(strings.a);
    strings.b = RemoveLeadingAndTrailingSpaces(strings.b);
    return strings;
}

#ifdef _WIN32
void B_Sleep(uint64_t millis)
{
    Sleep((DWORD)millis);
}
#else
#error "Unimplemented for this OS!"
#endif

#ifdef _WIN32
void DebugMessage(const char* message)
{
    OutputDebugString(message);
}
#else
#error "Unimplemented for this OS!"
#endif

void DebugMessageFmt(const char* fmt, ...)
{
    // MSVC doesn't have vasprintf, so we'll have to get
    // a bit creative...
    va_list args;
    va_start(args, fmt);
    int size = vsnprintf(nullptr, 0, fmt, args);  // Extra call to get the size of the string
    va_end(args);
    
    char* str = (char*)malloc(size+1);
    defer { free(str); };
    
    va_start(args, fmt);
    vsnprintf(str, size+1, fmt, args);
    va_end(args);
    
    DebugMessage(str);
}

#ifdef _WIN32
bool B_IsDebuggerPresent()
{
    return IsDebuggerPresent();
}
#else
#error "Unimplemented for this OS!"
#endif