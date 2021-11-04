#ifndef ASSIGNMENT_SIMCL_H
#define ASSIGNMENT_SIMCL_H

#include <cl.hpp>
#include <glm/glm.hpp>

#include <cmath>

typedef cl_float2 float2;
typedef cl_float3 float3;
typedef cl_float4 float4;
typedef uint32_t uint;
typedef uint64_t ulong;
typedef uint16_t ushort;

#define __global
#define __kernel

static inline thread_local volatile int workItemId[16];

inline void set_global_id(uint dimindx, uint value) {
    workItemId[dimindx] = value;
}

inline int get_global_id(uint dimindx) {
    return workItemId[dimindx];
}

inline float4 min(const float4 &a, const float4 &b) {
    return float4{std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z), std::min(a.w, b.w)};
}

inline float max(float a, float b) {
    return std::max(a, b);
}

inline float min(float a, float b) {
    return std::min(a, b);
}

inline float4 max(const float4 &a, const float4 &b) {
    return float4{std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z), std::max(a.w, b.w)};
}

inline float4 operator-(const float4 &lhs, const float4 &rhs) {
    return float4{lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z, lhs.w - rhs.w};
}

inline float4 operator+(const float4 &lhs, const float4 &rhs) {
    return float4{lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z, lhs.w + rhs.w};
}

inline float4 operator+(const float4 &lhs, float rhs) {
    return float4{lhs.x + rhs, lhs.y + rhs, lhs.z + rhs, lhs.w + rhs};
}

inline float4 operator+(float lhs, const float4 &rhs) {
    return float4{lhs + rhs.x, lhs + rhs.y, lhs + rhs.z, lhs + rhs.w};
}

inline float4& operator+=(float4 &lhs, const float4 &rhs) {
    lhs.x += rhs.x;
    lhs.y += rhs.y;
    lhs.z += rhs.z;
    lhs.w += rhs.w;
    return lhs;
}

inline float4 operator*(const float4 &lhs, const float4 &rhs) {
    return float4{lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z, lhs.w + rhs.w};
}

inline float4 operator*(float lhs, const float4 &rhs) {
    return float4{rhs.x * lhs, rhs.y * lhs, rhs.z * lhs, rhs.w * lhs};
}

inline float4 operator*(const float4 &lhs, float rhs) {
    return float4{lhs.x * rhs, lhs.y * rhs, lhs.z * rhs, lhs.w * rhs};
}

inline float4& operator*=(float4 &lhs, const float4 &rhs) {
    lhs.x *= rhs.x;
    lhs.y *= rhs.y;
    lhs.z *= rhs.z;
    lhs.w *= rhs.w;
    return lhs;
}

inline float4 operator/(float lhs, const float4 &rhs) {
    return float4{lhs / rhs.x, lhs / rhs.y, lhs / rhs.z, lhs / rhs.w};
}

inline float4 operator/(const float4 &lhs, float rhs) {
    return float4{lhs.x / rhs, lhs.y / rhs, lhs.z / rhs, lhs.w / rhs};
}

inline float4& operator/=(float4 &lhs, const float4 &rhs) {
    lhs.x /= rhs.x;
    lhs.y /= rhs.y;
    lhs.z /= rhs.z;
    lhs.w /= rhs.w;
    return lhs;
}

inline float4 operator+(const float4 &f4) {
    return f4;
}

inline float4 operator-(const float4 &f4) {
    return float4{-f4.x, -f4.y, -f4.z, -f4.w};
}

inline float2 operator+(const float2 &lhs, const float2 &rhs) {
    return float2{lhs.x + rhs.x, lhs.y + rhs.y};
}

inline float2 operator*(const float2 &lhs, float rhs) {
    return float2{lhs.x * rhs, lhs.y * rhs};
}

inline float4 toFloat4(const glm::vec4 &vec) {
    return float4{vec.x, vec.y, vec.z, vec.w};
}

inline float3 toFloat3(const glm::vec3 &vec) {
    return float3{vec.x, vec.y, vec.z, 1.0f};
}

inline float length(const float3 &vec) {
    return std::sqrt(vec.x * vec.x + vec.y * vec.y + vec.z * vec.z);
}

inline float3 normalize(const float3 &vec) {
    float len = length(vec);
    return float3{vec.x / len, vec.y / len, vec.z / len, vec.w};
}

inline float dot(const float3 &lhs, const float3 &rhs) {
    return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
}

inline float dot(const float2 &lhs, const float2 &rhs) {
    return lhs.x * rhs.x + lhs.y * rhs.y;
}

inline float fract(float v) {
    return v - floor(v);
}

inline float3 cross(const float3 &lhs, const float3 &rhs) {
    return float3{
        lhs.y * rhs.z - lhs.z * rhs.y,
        lhs.z * rhs.x - lhs.x * rhs.z,
        lhs.x * rhs.y - lhs.y * rhs.x,
    };
}

inline float3 vec3(float a) {
    return float3{a, a, a};
}

inline float3 vec3(float a, float b, float c) {
    return float3{a, b, c};
}

inline float4 vec4(float a) {
    return float4{a, a, a, a};
}

inline float4 vec4(float a, float b, float c, float d) {
    return float4{a, b, c, d};
}

inline float4 vec4(const float3& v, float d) {
    return float4{v.x, v.y, v.z, d};
}

inline float clamp(float v, float min, float max) {
    return v > max ? max : (v < min ? min : v);
}

template<typename T1, typename T2, typename P>
inline decltype(auto) mix(T1 &&a, T2 &&b, P &&p) {
    return a + (b - a) * p;
}

using std::isfinite;

#endif //ASSIGNMENT_SIMCL_H
