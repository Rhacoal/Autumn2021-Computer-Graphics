#include "lib/shaders/rt_definition.h"

int next(ulong *seed, int bits) {
    *seed = (*seed * 0x5DEECE66DL + 0xBL) & (((ulong) 1ul << 48) - 1);
    return (int) (*seed >> (48 - bits));
}

float randomFloat(ulong *seed) {
    return (float) next(seed, 24) / (float) (1 << 24);
}

/**
 * Checks if a ray intersects with a triangle.
 * @param ray the ray to check
 * @param triangle the triangle to check
 * @param collision outputs collision info if intersects
 * @return whether the ray and the triangle intersects
 */
bool intersect(Ray ray, Triangle triangle, Intersection *intersection) {
    // Reference:
    // https://scratchapixel.com/lessons/3d-basic-rendering/ray-tracing-rendering-a-triangle/moller-trumbore-ray-triangle-intersection

    float3 e01 = triangle.v1.position - triangle.v0.position;
    float3 e02 = triangle.v2.position - triangle.v0.position;

    float3 pvec = cross(ray.direction, e02);
    float det = dot(e01, pvec);
    if (fabs(det) < 1e-3) return false;
    float invDet = 1 / det;

    float3 tvec = ray.origin - triangle.v0.position;
    float u = dot(tvec, pvec) * invDet;
    if (u < 0 || u > 1) return false;

    float3 qvec = cross(tvec, e01);
    float v = dot(ray.direction, qvec) * invDet;
    if (v < 0 || u + v > 1) return false;

    float t = dot(e02, qvec) * invDet;
    if (t < 0) return false;

    intersection->barycentric.x = 1 - u - v;
    intersection->barycentric.y = u;
    intersection->barycentric.z = v;
    intersection->distance = t;
    intersection->position = ray.origin + t * ray.direction;
    intersection->side = det < 0;
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
    float3 bounds[2] = {bounds3.pMin, bounds3.pMax};
    float tmin, tmax, tymin, tymax, tzmin, tzmax;

    int sign[3];
    sign[0] = ray.direction.x > 0;
    sign[1] = ray.direction.y > 0;
    sign[2] = ray.direction.z > 0;

    tmin = (bounds[1 - sign[0]].x - ray.origin.x) * inverseRay.x;
    tmax = (bounds[sign[0]].x - ray.origin.x) * inverseRay.x;
    tymin = (bounds[1 - sign[1]].y - ray.origin.y) * inverseRay.y;
    tymax = (bounds[sign[1]].y - ray.origin.y) * inverseRay.y;

    if ((tmin > tymax) || (tymin > tmax))
        return false;
    if (tymin > tmin)
        tmin = tymin;
    if (tymax < tmax)
        tmax = tymax;

    tzmin = (bounds[1 - sign[2]].z - ray.origin.z) * inverseRay.z;
    tzmax = (bounds[sign[2]].z - ray.origin.z) * inverseRay.z;

    if ((tmin > tzmax) || (tzmin > tmax))
        return false;
    if (tzmin > tmin)
        tmin = tzmin;
    if (tzmax < tmax)
        tmax = tzmax;

    return true;
}

bool firstIntersection(Ray ray, __global BVHNode *bvh, __global Triangle *primitives, Intersection *output) {
    // TODO: fix possible stack overflow
    // probably no need...
    uint stack[64];
    stack[0] = 0;
    uint stackSize = 1;
    Intersection intersection;
    float maxT = 1e20;
    bool hasIntersection = false;
    float mss = 0;
    while (stackSize) {
        mss = max(mss, (float) stackSize);
        uint nodeIdx = stack[--stackSize];
        BVHNode node = bvh[nodeIdx];
        if (node.isLeaf) {
            if (intersect(ray, primitives[node.offset], &intersection)) {
                if (intersection.distance < maxT) {
                    maxT = intersection.distance;
                    intersection.index = node.offset;
                    *output = intersection;
                }
                hasIntersection = true;
            }
        } else {
            int sign[3];
            sign[0] = ray.direction.x > 0;
            sign[1] = ray.direction.y > 0;
            sign[2] = ray.direction.z > 0;

            if (stackSize > 32) {
                // too shallow, limit width
                uint t[2] = {nodeIdx + 1, node.offset};
                if (boundsRayIntersects(ray, bvh[t[sign[node.dim]]].bounds)) {
                    stack[stackSize++] = t[sign[node.dim]];
                } else if (boundsRayIntersects(ray, bvh[t[1 - sign[node.dim]]].bounds)) {
                    stack[stackSize++] = t[1 - sign[node.dim]];
                }
            } else {
                if (boundsRayIntersects(ray, bvh[nodeIdx + 1].bounds)) {
                    stack[stackSize++] = nodeIdx + 1;
                }
                if (boundsRayIntersects(ray, bvh[node.offset].bounds)) {
                    stack[stackSize++] = node.offset;
                }
            }
        }
    }
    if (!hasIntersection) {
        output->distance = mss;
    }

    return hasIntersection;
}

__kernel void raygeneration_kernel(
    __global Ray *output,
    uint width, uint height, ulong globalSeed,
    float3 cameraPosition, float3 cameraDir, float3 cameraUp, float fov, float near
) {
    const uint pixel_id = get_global_id(0);
    ulong seed = globalSeed ^ (pixel_id * globalSeed);
    seed = next(&seed, 48);
    float pixel_x = pixel_id % width + randomFloat(&seed);
    float pixel_y = pixel_id / width + randomFloat(&seed);
    float ndc_x = (pixel_x / width) * 2.0f - 1.0f;
    float ndc_y = (pixel_y / height) * 2.0f - 1.0f;
    float aspect = (float) width / (float) height;
    float top = near * tan(fov / 2);
//    float top = 1.0f / (near * tan(fov));
#ifdef __cplusplus
    float3 near_pos = float3{ndc_x * top * aspect, ndc_y * top, -near};
#else
    float3 near_pos = (float3) (ndc_x * top * aspect, ndc_y * top, -near);
#endif
    float3 cameraX = cross(cameraDir, cameraUp);
    float3 cameraY = cross(cameraX, cameraDir);
    float3 world_pos = near_pos.x * cameraX + near_pos.y * cameraY + near_pos.z * (-cameraDir) + cameraPosition;

    Ray ray;
    ray.origin = cameraPosition;
    ray.direction = normalize(world_pos - cameraPosition);
    output[pixel_id] = ray;
}

__kernel void clear_kernel(
    __global float4 *output, uint width, uint height
) {
#ifdef __cplusplus
    output[get_global_id(0)] = float4{0.0f, 0.0f, 0.0f, 0.0f};
#else
    output[get_global_id(0)] = (float4) (0.0f, 0.0f, 0.0f, 0.0f);
#endif
}

__kernel void accumulate_kernel(
    __global float4 *input, uint width, uint height, uint spp,
    __global float4 *output
) {
    const uint pixel_id = get_global_id(0);
    output[pixel_id] = input[pixel_id] / spp;
}

float3 brdf(__global RayTracingMaterial *materials,
            float3 normal, float3 position, float2 texcoord,
            float3 wi, float3 wo) {
#ifdef __cplusplus
    float x = dot(wi, normal) / RT_M_PI_F;
    return float3{x, x, x};
#else
    return (float3) (dot(wi, wo));
#endif
}

__kernel void render_kernel(
    __global float4 *output, uint width, uint height,
    __global BVHNode *bvh, __global Triangle *triangles, __global RayTracingMaterial *materials,
    __global Ray *rays, uint bounces,
    ulong globalSeed, uint spp
) {
    const uint pixel_id = get_global_id(0);
    Ray ray = rays[pixel_id];
    ulong seed = pixel_id ^ (globalSeed * pixel_id);
#ifdef __cplusplus
    float3 sum = {0.0f, 0.0f, 0.0f};
    float3 color = {0.0f, 0.0f, 0.0f};
#else
    float3 sum = (float3) (0.0f);
    float3 color = (float3) (0.0f);
#endif

    // a maximum of 15 bounces is allowed
    struct {
        uint mtlIndex;
        uint primtiveIndex;
        float3 wo;
        float3 wi;
        float3 p;
        float3 normal;
        float2 texcoord;
        float distance;
        float pdf;
        bool side;
        bool isSky;
    } stack[16];
    int bcnt = 0;
    Intersection intersection;
    intersection.distance = 0.0f;
    for (uint i = 0; i <= bounces; ++i) {
        if (firstIntersection(ray, bvh, triangles, &intersection)) {
            bcnt += 1;
            float3 wo = -ray.direction;
            float3 pos = intersection.position;
            stack[i].wo = wo;
            stack[i].p = pos;
            stack[i].mtlIndex = triangles[intersection.index].mtlIndex;
            stack[i].primtiveIndex = intersection.index;
            stack[i].distance = intersection.distance;
            stack[i].side = intersection.side;

            // select next position
            float a = randomFloat(&seed), b = randomFloat(&seed);
            float phi = RT_M_PI_F * 2.0f * a, theta = acos(b);
            Triangle triangle = triangles[intersection.index];
            float3 w = normalize(
                triangle.v0.normal * intersection.barycentric.x +
                triangle.v1.normal * intersection.barycentric.y +
                triangle.v2.normal * intersection.barycentric.z
            );
            stack[i].normal = w;
            stack[i].texcoord = (
                triangles->v0.texcoord * intersection.barycentric.x +
                triangles->v1.texcoord * intersection.barycentric.y +
                triangles->v2.texcoord * intersection.barycentric.z
            );
            if (intersection.side) w = -w;
#ifdef __cplusplus
            float3 temp = w.x > 0.1 ? float3{0.0f, 1.0f, 0.0f} : float3{1.0f, 0.0f, 0.0f};
#else
            float3 temp = w.x > 0.1 ? (float3)(0.0f, 1.0f, 0.0f) : (float3)(1.0f, 0.0f, 0.0f);
#endif
            float3 u = normalize(cross(temp, w));
            float3 v = cross(u, w);
            float3 next = w * b +
                          u * sin(theta) * cos(phi) +
                          v * sin(theta) * sin(phi);
            stack[i].pdf = RT_M_1_PI * 0.5f; // pdf(phi, theta) = 1 / 2pi
            stack[i].wi = next;
            ray.origin = pos + next * 0.001f; // avoid self-intersection
            ray.direction = next;
            stack[i].isSky = false;
        } else {
            // TODO draw sky
            ++bcnt;
            if (bcnt == 1) {
                output[pixel_id].x += intersection.distance / 5.0f;
                output[pixel_id].y += intersection.distance / 5.0f;
                output[pixel_id].z += intersection.distance / 5.0f;
                output[pixel_id].w += 1.0f;
                return;
            }
            stack[i].isSky = true;
            break;
        }
    }

    for (int i = bcnt - 1; i >= 0; --i) {
        if (stack[i].isSky) {
            // TODO IBL sky
#ifdef __cplusplus
            color = float3{0.0f, 0.046f, 0.311f};
#else
            color = (float3) (0.0f, 0.046f, 0.311f);
#endif
        } else {
            __global RayTracingMaterial *material = materials + triangles[stack[i].primtiveIndex].mtlIndex;
            // TODO finish brdf
            float3 contrib = color * brdf(
                material,
                stack[i].p, stack[i].normal, stack[i].texcoord,
                stack[i].wi, stack[i].wo
            ) * dot(stack[i].normal, stack[i].wi) / stack[i].pdf;
            color = contrib + material->emmision;
        }
    }
//    color = (stack[0].normal * 0.5f + 0.5f);
    float times = max(0.0f, (float) bcnt - 2.0f) / ((float) bounces - 1.0f);

    uint x = pixel_id % width;
    uint y = pixel_id / width;
    float fx = (float) x / (float) width;
    float fy = (float) y / (float) height;

#ifdef __cplusplus
    if (std::isfinite(color.x) && std::isfinite(color.y) && std::isfinite(color.z)) {
        output[pixel_id] = output[pixel_id] + float4{color.x, color.y, color.z, 1.0f};
    }
#else
    if (isfinite(color.x) && isfinite(color.y) && isfinite(color.z)) {
        output[pixel_id] += (float4) (color, 1.0f);
    }
#endif
}

__kernel void test_kernel(__global float4 *output, uint width, uint height) {
    uint pixel_id = get_global_id(0);
    uint x = pixel_id % width;
    uint y = pixel_id / width;
    float fx = (float) x / (float) width;
    float fy = (float) y / (float) height;
    output[pixel_id] =
#ifdef __cplusplus
        float4{fx, fy, 0.0f, 1.0f};
#else
    (float4)(fx, fy, 0, 1.0);
#endif
}
