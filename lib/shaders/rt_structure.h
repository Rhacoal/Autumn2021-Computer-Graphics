#ifndef RT_STRUCTURE_H
#define RT_STRUCTURE_H

#ifdef __cplusplus

#include <cl.hpp>
#include <simcl.h>

#endif

typedef struct RayTracingMaterial {
    float4 color;
    float ior;
} RayTracingMaterial;

typedef struct RayTracingLight {
    float4 color;
} RayTracingLight;

typedef struct Vertex {
    float3 position;
    float3 normal;
    float3 texcoord;
    float3 tangent;
} Vertex;

typedef struct Light {
    float3 position;
    float3 color;
    float intensity;
    float padding[3];
} Light;

typedef struct Bounds3 {
    float3 pMin, pMax;

#ifdef __cplusplus

    static constexpr auto MIN_FLOAT = std::numeric_limits<float>::min();
    static constexpr auto MAX_FLOAT = std::numeric_limits<float>::min();

    Bounds3() : pMin{MAX_FLOAT, MAX_FLOAT, MAX_FLOAT}, pMax{MIN_FLOAT, MIN_FLOAT, MIN_FLOAT} {}

    explicit Bounds3(float3 point) : pMin(point), pMax(point) {}

    Bounds3(float3 pMin, float3 pMax) : pMin(pMin), pMax(pMax) {}

    Bounds3 operator+(const Bounds3 &rhs) const {
        return Bounds3{min(pMin, rhs.pMin), max(pMax, rhs.pMax)};
    }

    Bounds3 &operator+=(const Bounds3 &rhs) {
        pMin = min(pMin, rhs.pMin);
        pMax = max(pMax, rhs.pMax);
        return *this;
    }

    Bounds3 operator+(const float3 &rhs) const {
        return Bounds3{min(pMin, rhs), max(pMax, rhs)};
    }

    Bounds3 &operator+=(const float3 &rhs) {
        pMin = min(pMin, rhs);
        pMax = max(pMax, rhs);
        return *this;
    }

    uint8_t maxExtent() const {
        auto t = pMax - pMin;
        return t.x > t.y ? (t.x > t.z ? 0 : 2) : (t.y > t.z ? 1 : 2);
    }

    float3 centroid() const {
        return (pMax + pMin) * 0.5f;
    }

#endif
} Bounds3;

typedef struct Triangle {
    Vertex v0, v1, v2;
    uint mtlIndex;

#ifdef __cplusplus

    Bounds3 bounds() const {
        Bounds3 ret(v0.position);
        ret += v1.position;
        ret += v2.position;
        return ret;
    }

#endif
} Triangle;

typedef struct BVHNode {
    Bounds3 bounds;
    uint offset;
    ushort isLeaf;
    ushort dim;
} BVHNode;

typedef struct Ray {
    float3 origin;
    float3 direction;
    char sign[4];
    float possibility;
} Ray;

typedef struct Intersection {
    float3 position;
    float3 barycentric;
    uint index;
    float distance;
    // false if the ray is from outside or true if the ray is from inside
    bool side;
} Intersection;

#endif //RT_STRUCTURE_H
