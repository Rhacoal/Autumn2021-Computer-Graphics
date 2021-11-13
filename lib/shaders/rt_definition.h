#ifndef ASSIGNMENT_RT_DEFINITION_H
#define ASSIGNMENT_RT_DEFINITION_H

#include "lib/shaders/rt_structure.h"

bool intersect(Ray, Triangle, Intersection *);

bool boundsRayIntersects(Ray, Bounds3, float *, float *);

bool firstIntersection(Ray, __global BVHNode *, __global Triangle *, Intersection *);

__kernel void raygeneration_kernel(
    __global float3 *output,
    uint width, uint height, uint spp,
    __global ulong *globalSeed,
    float3 cameraPosition, float3 cameraDir, float3 cameraUp, float fov, float near
);

__kernel void render_kernel(
    // output image
    __global float4 *output, uint width, uint height,
    // primitives and materials
    __global BVHNode *bvh, __global Triangle *triangles, __global RayTracingMaterial *materials,
    // light tracing
    __global const uint *lights, uint lightCount,
    // path tracing
    __global float3 *rays, float3 cameraPosition, uint bounces,
    // sampling
    __global ulong *globalSeed, uint spp
);

#endif //ASSIGNMENT_RT_DEFINITION_H
