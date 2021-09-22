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

#ifdef __cplusplus
struct RenderKernelArgs {
    constexpr static uint output = 0;
    constexpr static uint width = 1;
    constexpr static uint height = 2;
    constexpr static uint bvh = 3;
    constexpr static uint triangles = 4;
    constexpr static uint materials = 5;
    constexpr static uint textures = 6;
    constexpr static uint textureImage = 7;
    constexpr static uint lights = 8;
    constexpr static uint lightCount = 9;
    constexpr static uint rays = 10;
    constexpr static uint cameraPosition = 11;
    constexpr static uint bounces = 12;
    constexpr static uint globalSeed = 13;
    constexpr static uint spp = 14;
};
#endif

__kernel void render_kernel(
    // output image
    __global float4 *output, uint width, uint height,
    // primitives and materials
    __global BVHNode *bvh, __global Triangle *triangles, __global RayTracingMaterial *materials,
    __global RayTracingTextureRange *textures, __global float4 *textureImage,
    // light tracing
    __global const uint *lights, uint lightCount,
    // path tracing
    __global float3 *rays, float3 cameraPosition, uint bounces,
    // sampling
    __global ulong *globalSeed, uint spp
);

#endif //ASSIGNMENT_RT_DEFINITION_H
