#ifndef ASSIGNMENT_SIMCL_H
#define ASSIGNMENT_SIMCL_H

#include <cl.hpp>

typedef cl_float2 float2;
typedef cl_float3 float3;
typedef cl_float4 float4;
typedef uint32_t uint;

#define __global
#define __kernel

int get_global_id(uint dimindx);

inline float4 min(const float4 &a, const float4 &b) {
    return float4{std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z), std::min(a.w, b.w)};
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

inline float4 operator*(const float4 &lhs, const float4 &rhs) {
    return float4{lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z, lhs.w + rhs.w};
}

inline float4 operator*(const float4 &lhs, float rhs) {
    return float4{lhs.x * rhs, lhs.y * rhs, lhs.z * rhs, lhs.w * rhs};
}

inline float4 operator/(const float4 &lhs, float rhs) {
    return float4{lhs.x / rhs, lhs.y / rhs, lhs.z / rhs, lhs.w / rhs};
}

#endif //ASSIGNMENT_SIMCL_H
