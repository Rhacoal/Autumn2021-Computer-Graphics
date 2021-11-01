#include "lib/shaders/rt_structure.h"

int next(ulong *seed, int bits) {
    *seed = (*seed * 0x5DEECE66DL + 0xBL) & (((ulong) 1ul << 48) - 1);
    return (int) (*seed >> (48 - bits));
}

float randomFloat(ulong *seed) {
    return (float) next(seed, 24) / (float) (1 << 24);
}

typedef struct Intersection {
    float3 position;
    float3 barycentric;
    // false if the ray is from outside or true if the ray is from inside
    bool side;
    uint index;
    float distance;
} Intersection;

/**
 * Checks if a ray intersects with a triangle.
 * @param ray the ray to check
 * @param triangle the triangle to check
 * @param collision outputs collision info if intersects
 * @return whether the ray and the triangle intersects
 */
bool intersect(Ray ray, Triangle triangle, Intersection *intersection) {
    float3 e01 = triangle.v1.position - triangle.v0.position;
    float3 e02 = triangle.v2.position - triangle.v0.position;
    float3 normal = cross(e01, e02);
    float area = length(normal) * 0.5f;
    float nl = dot(ray.direction, normal);
    if (abs(nl) < 0.001f) { // parallel
        return false;
    }

    float d = dot(triangle.v0.position - ray.origin, normal) / nl;
    if (d < 0) { // triangle behind ray
        return false;
    }
    float3 p = ray.origin + ray.direction * d;
    float3 e0p = p - triangle.v0.position;
    float invArea2 = 1.0f / normal;

    w = cross(e01, e0p) * invArea2;
    if (w < 0) { // v0P is right to v0v1
        return false;
    }

    v = cross(e0p, e02) * invArea2;
    if (v < 0) { // v0P is left to v0v2
        return false;
    }

    float u = 1 - w - v;
    if (u < 0) { // P is too far from v0
        return false;
    }

    intersection->barycentric.x = u;
    intersection->barycentric.y = v;
    intersection->barycentric.z = w;
    intersection->position = p;
    intersection->side = nl > 0;
    return true;
}

bool boundsRayIntersects(Ray ray, Bounds3 bounds3) {
#ifdef __cplusplus
    float3 inverseRay = 1.0f / ray.direction;
#else
    float3 inverseRay = (float3)(1.0f, 1.0f, 1.0f) / ray.direction;
#endif
    // Reference:
    // https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-box-intersection
    float3 bounds[2] = {bounds.pMin, bounds.pMax};
    float tmin, tmax, tymin, tymax, tzmin, tzmax;

    tmin = (bounds[ray.sign[0]].x - ray.origin.x) * inverseRay.x;
    tmax = (bounds[1 - ray.sign[0]].x - ray.origin.x) * inverseRay.x;
    tymin = (bounds[ray.sign[1]].y - ray.origin.y) * inverseRay.y;
    tymax = (bounds[1 - ray.sign[1]].y - ray.origin.y) * inverseRay.y;

    if ((tmin > tymax) || (tymin > tmax))
        return false;
    if (tymin > tmin)
        tmin = tymin;
    if (tymax < tmax)
        tmax = tymax;

    tzmin = (bounds[r.sign[2]].z - ray.origin.z) * inverseRay.z;
    tzmax = (bounds[1 - r.sign[2]].z - ray.origin.z) * inverseRay.z;

    if ((tmin > tzmax) || (tzmin > tmax))
        return false;
    if (tzmin > tmin)
        tmin = tzmin;
    if (tzmax < tmax)
        tmax = tzmax;

    return true;
}

bool firstIntersection(Ray ray, BVHNode *bvh, Triangle *primitives, Intersection *output) {
    // TODO: fix possible stack overflow
    uint stack[64], stackSize;
    stack[0] = 0;
    stackSize = 1;
    Intersection intersection;
    float maxT = 1e20;
    bool hasIntersection = false;
    while (stackSize) {
        uint nodeIdx = stack[--stackSize];
        BVHNode node = bvh[nodeIdx];
        if (node.isLeaf) {
            if (intersect(ray, primitives[node.offset], &intersection)) {
                if (intersection.distance < maxT) {
                    maxT = intersection.distance;
                    *output = intersection;
                }
                hasIntersection = true;
            }
        }
        if (stackSize > 32) {
            // too shallow, limit width
            uint t[2] = {nodeIdx + 1, node.offset};
            if (boundsRayIntersects(ray, bvh[t[ray.sign[node.dim]]].bounds)) {
                stack[++stackSize] = t[ray.sign[node.dim]];
            } else if (boundsRayIntersects(ray, bvh[t[1 - ray.sign[node.dim]]].bounds)) {
                stack[++stackSize] = t[1 - ray.sign[node.dim]];
            }
        } else {
            if (boundsRayIntersects(ray, bvh[nodeIdx + 1].bounds)) {
                stack[++stackSize] = nodeIdx + 1;
            }
            if (boundsRayIntersects(ray, bvh[node.offset])) {
                stack[++stackSize] = node.offset;
            }
        }
    }
    return hasIntersection;
}

__kernel void generateRays(
    __global Ray *output,
    uint width, uint height, ulong globalSeed,
    // cameraZ = -dir, cameraX = cross(dir, up), cameraY = cross(z, x)
    float3 cameraPosition, float3 cameraX, float3 cameraY, float3 cameraZ, float fov, float near
) {
    const int pixel_id = get_global_id(0);
    ulong seed = pixel_id ^ globalSeed;
    float pixel_x = pixel_id % width + randomFloat(&seed);
    float pixel_y = pixel_id / width + randomFloat(&seed);
    float ndc_x = (pixel_x / width) * 2.0f - 1.0f;
    float ndc_y = (pixel_y / height) * 2.0f - 1.0f;
    float aspect = (float) height / (float) width;
    float top = 1.0f / (near * tan(fov));
#ifdef __cplusplus
    float3 near_pos = float3{ndc_x * top * aspect, ndc_y * top, -near};
#else
    float3 near_pos = (float3) (ndc_x * top * aspect, ndc_y * top, -near);
#endif
    float3 world_pos = near_pos.x * cameraX + near_pos.y * cameraY + near_pos.z * cameraZ;

    Ray ray;
    ray.origin = cameraPosition;
    ray.direction = normalize(world_pos - cameraPosition);
    ray.sign[0] = ray.direction.x > 0;
    ray.sign[1] = ray.direction.y > 0;
    ray.sign[2] = ray.direction.z > 0;
    ray.possibility = 1.0f;
}

__kernel void render_kernel(
    // output image
    __global float4 *output, uint width, uint height,
    // primitives and materials
    __global BVHNode *bvh, __global Triangle *triangles, __global RayTracingMaterial *materials,
    // lighting
    __global Light *lights, uint lightCount,
    // sky
    __global Skybox *skybox,
    // path tracing
    __global Rays *rays, uint bounces,
    // sampling
    ulong globalSeed, uint spp
) {
    const int pixel_id = get_global_id(0);
    Ray ray = rays[pixel_id];
#ifdef __cplusplus
    float3 color = {0.0f, 0.0f, 0.0f};
#else
    float3 color = (float3) (0.0f);
#endif

    // a maximum of 15 bounces is allowed
    float3 stack[16][2];
    Intersection intersection;
    if (firstIntersection(ray, bvh, triangles, &intersection)) {
        // iteratively select directions
        for (int i = 0; i < bounces; ++i) {
            float3 wo = -ray.direction;
            float3 p = intersection.position;
            RayTracingMaterial *mtl = materials[triangles[intersection.index].mtlIndex];
        }
    } else {
        // TODO draw sky
    }

    uint x = pixel_id % width;
    uint y = pixel_id / width;
    float fx = (float) x / (float) width;
    float fy = (float) y / (float) height;
    output[pixel_id] =
#ifdef __cplusplus
        float4{fx, fy, 0f, 1.0f};
#else
    (float4)(fx, fy, 0, 1.0);
#endif
}
