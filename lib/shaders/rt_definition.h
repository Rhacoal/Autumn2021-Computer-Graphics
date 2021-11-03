#ifndef ASSIGNMENT_RT_DEFINITION_H
#define ASSIGNMENT_RT_DEFINITION_H

#include "lib/shaders/rt_structure.h"

#define RT_M_PI_F           3.14159274101257f
#define RT_M_PI             3.141592653589793115998
#define RT_M_1_PI_F         0.31830987334251f
#define RT_M_1_PI           0.318309886183790691216

bool intersect(Ray, Triangle, Intersection *);

bool boundsRayIntersects(Ray, Bounds3);

bool firstIntersection(Ray, __global BVHNode *, __global Triangle *, Intersection *);

float3 brdf(__global RayTracingMaterial *materials,
            float3 normal, float3 position, float2 texcoord,
            float3 wi, float3 wo);

__kernel void raygeneration_kernel(
    __global Ray *output,
    uint width, uint height, ulong globalSeed,
    float3 cameraPosition, float3 cameraDir, float3 cameraUp, float fov, float near
);

__kernel void render_kernel(
    // output image
    __global float4 *output, uint width, uint height,
    // primitives and materials
    __global BVHNode *bvh, __global Triangle *triangles, __global RayTracingMaterial *materials,
    // path tracing
    __global Ray *rays, uint bounces,
    // sampling
    ulong globalSeed, uint spp
);

#endif //ASSIGNMENT_RT_DEFINITION_H
