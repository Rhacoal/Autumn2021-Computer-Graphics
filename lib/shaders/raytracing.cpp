#include "lib/shaders/rt_definition.h"
#include "lib/shaders/rt_common.h"
#include "lib/shaders/bxdf.h"

float3 BxDF(__global RayTracingMaterial *material,
            float3 normal, float3 position, float2 texcoord,
            float3 wi, float3 wo) {
//    return vec3(dot(wi, normal)) * material->albedo * RT_M_1_PI_F;
    float3 reflection = Disney_BRDF(wi, wo, normal, material->albedo, 0.0f,
        material->metallic, 0.5f, 0.0f, material->roughness);
    return clamp(reflection, 0.0f, 1.0f);
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
    if (fabs(det) < 1e-5) return false;
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

    if ((tmin > tymax) || (tymin > tmax)) {
        return false;
    }
    if (tymin > tmin) {
        tmin = tymin;
    }
    if (tymax < tmax) {
        tmax = tymax;
    }

    tzmin = (bounds[1 - sign[2]].z - ray.origin.z) * inverseRay.z;
    tzmax = (bounds[sign[2]].z - ray.origin.z) * inverseRay.z;

    if ((tmin > tzmax) || (tzmin > tmax)) {
        return false;
    }
    if (tzmin > tmin) {
        tmin = tzmin;
    }
    if (tzmax < tmax) {
        tmax = tzmax;
    }

    return true;
}

bool firstIntersection(Ray ray, __global BVHNode *bvh, __global Triangle *primitives, Intersection *output) {
    uint stack[64];
    stack[0] = 0;
    uint stackSize = 1;
    Intersection intersection;
    float maxT = 1e20;
    float mss = 0.0f;
    bool hasIntersection = false;
    while (stackSize) {
        mss = max(mss, (float) stackSize);
        uint nodeIdx = stack[--stackSize];
        BVHNode node = bvh[nodeIdx];
        if (node.isLeaf) {
            if (intersect(ray, primitives[node.offset], &intersection)) {
                if (intersection.distance > 1e-5 && intersection.distance < maxT) {
                    maxT = intersection.distance;
                    intersection.index = node.offset;
                    intersection.position = ray.origin + ray.direction * intersection.distance;
                    *output = intersection;
                }
                hasIntersection = true;
            }
        } else {
            int sign[3];
            sign[0] = ray.direction.x > 0;
            sign[1] = ray.direction.y > 0;
            sign[2] = ray.direction.z > 0;

            uint t[2] = {nodeIdx + 1, node.offset};
            if (boundsRayIntersects(ray, bvh[t[sign[node.dim]]].bounds)) {
                stack[stackSize++] = t[sign[node.dim]];
            }
            if (boundsRayIntersects(ray, bvh[t[1 - sign[node.dim]]].bounds)) {
                stack[stackSize++] = t[1 - sign[node.dim]];
            }
        }
        if (stackSize >= 63) {
            break;
        }
    }
    output->padding = mss;

    return hasIntersection;
}

__kernel void raygeneration_kernel(
    __global float3 *output,
    uint width, uint height, uint spp,
    __global ulong *globalSeed,
    float3 cameraPosition, float3 cameraDir, float3 cameraUp, float fov, float near
) {
    const uint pixel_id = get_global_id(0);
    ulong seed = globalSeed[pixel_id];
    seed = next(&seed, 48);
    float px = (float) (pixel_id % width);
    float py = (float) (pixel_id / width);
    float aspect = (float) width / (float) height;
    float top = near * tan(fov / 2);
    float invWidth2 = 2.0f / (float) width;
    float invHeight2 = 2.0f / (float) height;
    float3 cameraX = normalize(cross(cameraDir, cameraUp));
    float3 cameraY = cross(cameraX, cameraDir);

    for (uint i = 0; i < spp; ++i) {
        float pixel_x = px + randomFloat(&seed);
        float pixel_y = py + randomFloat(&seed);
        // same as: float ndc_x = pixel_x / width * 2.0f - 1.0f
        float ndc_x = (pixel_x * invWidth2) - 1.0f;
        float ndc_y = (pixel_y * invHeight2) - 1.0f;
        float3 near_pos = vec3(ndc_x * top * aspect, ndc_y * top, -near);
        float3 world_pos = near_pos.x * cameraX + near_pos.y * cameraY + near_pos.z * (-cameraDir) + cameraPosition;
        output[pixel_id * spp + i] = normalize(world_pos - cameraPosition);
    }
    globalSeed[pixel_id] = seed;
}

__kernel void clear_kernel(
    __global float4 *output, uint width, uint height
) {
    output[get_global_id(0)] = vec4(0.0f, 0.0f, 0.0f, 0.0f);
}

__kernel void accumulate_kernel(
    __global float4 *input, uint width, uint height, uint spp,
    __global float4 *output
) {
    const uint pixel_id = get_global_id(0);
    output[pixel_id] = input[pixel_id] / (float) spp;
}

__kernel void render_kernel(
    __global float4 *output, uint width, uint height,
    __global BVHNode *bvh, __global Triangle *triangles,
    __global RayTracingMaterial *materials,
//    __global RayTracingTextureRange *textures, image2d_t textureImage,
    __global const uint *lights, uint lightCount,
    __global float3 *rays, float3 cameraPosition, uint bounces,
    __global ulong *globalSeed, uint spp
) {
    const uint pixelId = get_global_id(0);
    ulong seed = globalSeed[pixelId];
    float3 sum = vec3(0.0f);
    if (pixelId == width * (height / 2) + (width / 2)) {
        debugger;
    }

    for (uint sampleId = 0; sampleId < spp; ++sampleId) {
        float3 rayDir = rays[pixelId * spp + sampleId];
        Ray ray;
        ray.direction = rayDir;
        ray.origin = cameraPosition;
        float3 color = vec3(0.0f);

        // a maximum of 15 bounces is allowed
        struct {
            uint mtlIndex;
            uint primtiveIndex;
            float3 wo;
            float3 wi;
            float3 position;
            float3 normal;
            float2 texcoord;
            float3 light;
            float3 lightDir;
            float3 debug;
            Ray ray;
            float distance;
            float pdf;
            bool side;
            bool isSky;
            bool canReachLight;
        } stack[16];
        int bounceCount = 0;
        Intersection intersection;
        intersection.distance = 0.0f;
        for (uint i = 0; i <= bounces; ++i) {
            bounceCount += 1;
            if (firstIntersection(ray, bvh, triangles, &intersection)) {
                float3 wo = -ray.direction;
                float3 pos = intersection.position;
                stack[i].wo = wo;
                stack[i].position = pos;
                stack[i].mtlIndex = triangles[intersection.index].mtlIndex;
                stack[i].primtiveIndex = intersection.index;
                stack[i].distance = intersection.distance;
                stack[i].side = intersection.side;

                Triangle triangle = triangles[intersection.index];
                float3 normal = normalize(
                    triangle.v0.normal * intersection.barycentric.x +
                    triangle.v1.normal * intersection.barycentric.y +
                    triangle.v2.normal * intersection.barycentric.z
                );
                if (intersection.side) normal = -normal;
                float3 w = normal;
                stack[i].normal = normal;
                stack[i].texcoord = (
                    triangle.v0.texcoord * intersection.barycentric.x +
                    triangle.v1.texcoord * intersection.barycentric.y +
                    triangle.v2.texcoord * intersection.barycentric.z
                );

                // TODO: fresnel
                // __global RayTracingMaterial *material = materials + triangle.mtlIndex;
                // float R0 = pow2((1.0f - material->ior) / (1.0f + material->ior));
                // float Rtheta = R0 + (1.0f - R0) * pow5(dot(wo, w));

                // sample light
                if (lightCount) {
                    uint i0 = randomInt(&seed, lightCount);
                    uint triangleIdx = lights[i0];

                    Triangle lightTriangle = triangles[triangleIdx];
                    float s, t;
                    do {
                        s = randomFloat(&seed);
                        t = sqrt(randomFloat(&seed));
                    } while (s + t > 1);
                    float3 e01 = lightTriangle.v1.position - lightTriangle.v0.position;
                    float3 e02 = lightTriangle.v2.position - lightTriangle.v0.position;
                    float3 lightPos = lightTriangle.v0.position + s * e01 + t * e02;
                    Intersection lightRayIntersection;
                    Ray lightRay;
                    lightRay.direction = normalize(lightPos - pos);
                    lightRay.origin = pos + lightRay.direction;
                    bool intersectLight = firstIntersection(lightRay, bvh, triangles, &lightRayIntersection);
                    stack[i].debug = intersectLight ? vec3(
                        1.0f / (length(lightRayIntersection.position - lightPos) + 0.01f)) : vec3(1.0f, 0.0f, 0.0f);
                    if (intersectLight
                        // TODO: this side criteria might be unstable!
                        //                    && !lightRayIntersection.side
                        && lightRayIntersection.index == triangleIdx) {
                        stack[i].light = materials[lightTriangle.mtlIndex].emission
                                         * fabs(dot(lightTriangle.v0.normal, -lightRay.direction))
                                         / length(lightPos - pos)
                                         / (0.5f / length(cross(e01, e02))); // pdf_light
                        stack[i].lightDir = lightRay.direction;
                        stack[i].canReachLight = true;
                    } else {
                        stack[i].light = vec3(0.0f);
                        stack[i].canReachLight = false;
                    }
                } else {
                    stack[i].light = vec3(0.0f);
                    stack[i].canReachLight = false;
                }

                // select next position
                float a = randomFloat(&seed), b = randomFloat(&seed);
                float phi = RT_M_PI_F * 20.0f * a, theta = acos(b);
                float3 temp = fabs(w.x) > 0.1f ? vec3(0.0f, 1.0f, 0.0f) : vec3(1.0f, 0.0f, 0.0f);
                stack[i].pdf = RT_M_1_PI_F * 0.5f; // pdf(phi, theta) = 1 / 2pi
                float3 u = normalize(cross(temp, w));
                float3 v = cross(w, u);

                float3 next = normalize(w * b +
                                        u * sin(theta) * cos(phi) +
                                        v * sin(theta) * sin(phi));
                stack[i].wi = next;
                ray.origin = pos + next; // avoid self-intersection
                ray.direction = next;
                stack[i].ray = ray;
                stack[i].isSky = false;
            } else {
                // TODO draw sky
                stack[i].isSky = true;
                break;
            }
        }

        for (int i = bounceCount - 1; i >= 0; --i) {
            if (stack[i].isSky) {
                // TODO IBL sky
                color = vec3(0.0f, 0.023f, 0.1f);
            } else {
                __global RayTracingMaterial *material = materials + triangles[stack[i].primtiveIndex].mtlIndex;
                float3 contrib = color * BxDF(
                    material,
                    stack[i].normal, stack[i].position, stack[i].texcoord,
                    stack[i].wi, stack[i].wo
                ) * dot(stack[i].normal, stack[i].wi) / stack[i].pdf;
                if (stack[i].canReachLight) {
                    contrib += stack[i].light * BxDF(
                        material,
                        stack[i].normal, stack[i].position, stack[i].texcoord,
                        stack[i].lightDir, stack[i].wo
                    ) * dot(stack[i].normal, stack[i].lightDir);
                }
                stack[i].debug = contrib;
                color = contrib;
                if (i == 0) {
                    // to display lighting source
                    color += material->emission;
                }
            }
        }

        if (isfinite(color.x) && isfinite(color.y) && isfinite(color.z)) {
//            float max_v = fmax(color.x, fmax(color.y, color.z));
//            if (max_v > 100.0f) {
//                color /= (max_v / 100.0f);
//            }
            sum += color;
        }
//        if (sampleId == 1) {
//            sum = stack[0].debug;
//            break;
//        }
    }
    globalSeed[pixelId] = seed;
    output[pixelId] += vec4(sum, (float) spp);
}

__kernel void test_kernel(__global float4 *output, uint width, uint height) {
    uint pixel_id = get_global_id(0);
    uint x = pixel_id % width;
    uint y = pixel_id / width;
    float fx = (float) x / (float) width;
    float fy = (float) y / (float) height;
    output[pixel_id] = vec4(fx, fy, 0.0f, 1.0f);
}
