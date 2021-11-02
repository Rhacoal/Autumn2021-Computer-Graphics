#ifndef ASSIGNMENT_RT_DEFINITION_H
#define ASSIGNMENT_RT_DEFINITION_H

#include "lib/shaders/rt_structure.h"

int next(ulong *, int);

float randomFloat(ulong *);

bool intersect(Ray, Triangle, Intersection *);

bool boundsRayIntersects(Ray, Bounds3);

bool firstIntersection(Ray, __global BVHNode *, __global Triangle *, Intersection *);

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
