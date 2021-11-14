#include "lib/shaders/rt_definition.h"
#include "lib/shaders/rt_common.h"
#include "lib/shaders/bxdf.h"

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
    if (fabs(det) < 1e-5f) return false;
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

bool boundsRayIntersects(Ray ray, Bounds3 bounds3, float *tmin_out, float *tmax_out) {
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
    *tmin_out = tmin;
    *tmax_out = tmax;

    return true;
}

bool firstIntersection(Ray ray, __global BVHNode *bvh, __global Triangle *primitives, Intersection *output) {
    uint stack[64];
    stack[0] = 0;
    uint stackSize = 1;
    Intersection intersection;
    float maxT = 1e20f;
    float mss = 0.0f;
    bool hasIntersection = false;
    while (stackSize) {
        mss = max(mss, (float) stackSize);
        uint nodeIdx = stack[--stackSize];
        BVHNode node = bvh[nodeIdx];
        if (node.isLeaf) {
            if (intersect(ray, primitives[node.offset], &intersection)) {
                if (intersection.distance < 1e-5f) {

                } else if (intersection.distance < maxT) {
                    maxT = intersection.distance;
                    intersection.index = node.offset;
                    intersection.position = ray.origin + ray.direction * intersection.distance;
                    *output = intersection;
                    hasIntersection = true;
                }
            }
        } else {
            int sign[3];
            sign[0] = ray.direction.x > 0;
            sign[1] = ray.direction.y > 0;
            sign[2] = ray.direction.z > 0;

            uint t[2] = {nodeIdx + 1, node.offset};
            float tMin, tMax;
            if (boundsRayIntersects(ray, bvh[t[sign[node.dim]]].bounds, &tMin, &tMax)) {
                if (tMin <= maxT) {
                    stack[stackSize++] = t[sign[node.dim]];
                }
            }
            if (boundsRayIntersects(ray, bvh[t[1 - sign[node.dim]]].bounds, &tMin, &tMax)) {
                if (tMin <= maxT) {
                    stack[stackSize++] = t[1 - sign[node.dim]];
                }
            }
        }
        if (stackSize >= 63) {
            // prevent stack overflow
            break;
        }
    }
    output->padding = mss;

    return hasIntersection;
}

RayTracingMaterial evaluateMaterial(__global RayTracingMaterial *material, float2 texcoord) {
    return *material;
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
        float3 radiance = vec3(0.0f);

        // a maximum of 15 bounces is allowed
        struct {
            float3 position;
            float3 light;
            float3 emission;
            float3 transmission;
            float3 debug;
            uint primitiveIndex;
            float ior;
            bool isSky;
            bool canReachLight;
            bool side;
        } stack[16];
        int bounceCount = 0;
        Intersection intersection;
        intersection.distance = 0.0f;
        for (uint i = 0; i <= bounces; ++i) {
            bounceCount += 1;
            if (firstIntersection(ray, bvh, triangles, &intersection)) {
                if (i && stack[i - 1].primitiveIndex == intersection.index) {
                    // discard self-intersection
                    break;
                }
                float3 wo = -ray.direction;
                float3 pos = intersection.position;
                stack[i].debug = vec3(0.0f);
                stack[i].side = intersection.side;
                stack[i].position = pos;
                stack[i].primitiveIndex = intersection.index;

                Triangle triangle = triangles[intersection.index];
                float3 te01 = triangle.v1.position - triangle.v0.position;
                float3 te02 = triangle.v2.position - triangle.v0.position;
                float3 triangleNormal = normalize(cross(te01, te02));
                float3 normal = normalize(
                    triangle.v0.normal * intersection.barycentric.x +
                    triangle.v1.normal * intersection.barycentric.y +
                    triangle.v2.normal * intersection.barycentric.z
                );
                if (intersection.side) normal = -normal;
                float2 texcoord = (
                    triangle.v0.texcoord * intersection.barycentric.x +
                    triangle.v1.texcoord * intersection.barycentric.y +
                    triangle.v2.texcoord * intersection.barycentric.z
                );

                RayTracingMaterial material = evaluateMaterial(materials + triangle.mtlIndex, texcoord);
                stack[i].emission = intersection.side ? vec3(0.0f) : material.emission;

                float3 baseColor = material.albedo;
                float3 specularF0 = mix(vec3(0.04f), baseColor, material.metallic);
                float3 diffuseColor = baseColor * (1.0f - material.metallic);

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
                    if (intersectLight
                        && !lightRayIntersection.side
                        && lightRayIntersection.index == triangleIdx) {
                        float3 brdf = BRDF(lightRay.direction, -ray.direction, normal, material.roughness,
                            diffuseColor, specularF0, 1.0f);
                        stack[i].light = materials[lightTriangle.mtlIndex].emission
                                         * fabs(dot(lightTriangle.v0.normal, -lightRay.direction))
                                         / length(lightPos - pos)
                                         / (0.5f / length(cross(e01, e02))) // pdf_light
                                         * brdf
                                         * dot(normal, lightRay.direction);
                        stack[i].canReachLight = true;
                    } else {
                        stack[i].light = vec3(0.0f);
                        stack[i].canReachLight = false;
                    }
                } else {
                    stack[i].light = vec3(0.0f);
                    stack[i].canReachLight = false;
                }

                // TODO: fresnel

//                float metallicChance = clamp(material.metallic, 0.04f, 1.0f);
                float metallicChance = clamp(material.metallic, 0.0f, 1.0f);
                float refractionChance = (1.0f - metallicChance) * material.specTrans;
                float diffuseChance = 1.0f - metallicChance - refractionChance;

                float ev = randomFloat(&seed);
                float3 wi;
                float wo_ior = intersection.side ? material.ior : (i ? stack[i - 1].ior : 1.0f);
                float wi_ior = intersection.side ? 1.0f : material.ior; // might be wrong
                float next_ior = wo_ior; // if reflection
                float nn = wo_ior / wi_ior;
                if (ev < metallicChance) {
                    // specular reflection
                    float pdf;
                    wi = metallicBRDFSample(normal, wo, material.roughness, &pdf, &seed);
                    stack[i].transmission = SpecularBRDF(wi, wo, normal, material.roughness, specularF0)
                                            * dot(wi, normal) / pdf / metallicChance;
                } else if (ev < diffuseChance + metallicChance) {
                    // diffuse
                    float pdf;
                    wi = cosineWeightedHemisphereSample(normal, &pdf, &seed);
                    float R0 = IorToR0(dot(wi, normalize(wi + wo)), nn);
                    float3 diffuse = DiffuseBRDF(wi, wo, normal, material.roughness, diffuseColor, nn);
                    float3 specular = SpecularBRDF(wi, wo, normal, material.roughness, diffuseColor * R0);
                    stack[i].transmission = (diffuse + specular) * dot(wi, normal)
                                            / pdf / diffuseChance; // * (1.0f - material.specTrans);
                } else {
                    // (perfecct) refraction
                    // use normal as H
                    float3 H = normal;
                    float VdotH = dot(wo, H);
                    VdotH = fabs(dot(wo, H));
                    float sinVH2 = 1.0f - pow2(VdotH);
                    float sinLH2 = nn * nn * sinVH2;

                    float reflectionChance = 0.0f;
                    if (sinLH2 >= 1) {
                        reflectionChance = 1.0f;
                        // total reflection
                        wi = reflect(wo, H);
                        next_ior = wo_ior;
                        stack[i].transmission = diffuseColor / refractionChance * material.specTrans;
                    } else {
                        float cosLH = sqrt(1.0f - sinLH2);
                        float R0 = pow2((nn - 1.0f) / (nn + 1.0f));
                        float Fs = mix(R0, 1.0f, pow5(1.0f - cosLH));

                        if (randomFloat(&seed) < Fs) {
                            // pure specular reflection
                            next_ior = wo_ior;
                            wi = reflect(wo, H);
                            // pdf is the same as fresnel factor, no need to account for
                            stack[i].transmission = diffuseColor / refractionChance * material.specTrans;
                        } else {
                            // pure specular refraction
                            next_ior = wi_ior;
                            wi = normalize(-nn * wo + (nn * VdotH - cosLH) * H);

                            stack[i].transmission = diffuseColor / refractionChance * material.specTrans;
                        }
//                        stack[i].transmission = vec3(0.0f);
                    }
                }
//                stack[i].debug = vec3(clamp(intersection.distance * 0.25f, 0.0f, 1.0f));
//                stack[i].debug = vec3(0.0f, intersection.side ? 1.0f : 0.0f, intersection.side ? 0.0f : 1.0f);
//                stack[i].debug = intersection.barycentric;
//                stack[i].debug = normal * 0.5f + 0.5f;
//                stack[i].debug = wi * 0.5f + 0.5f;
//                stack[i].debug = wi * 0.5f + 0.5f;
//                stack[i].debug = vec3(0.5f, 0.5f, next_ior * 0.5f);

                stack[i].ior = next_ior;
                ray.origin = pos;
                ray.direction = wi;
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
//                radiance = vec3(0.0f, 0.023f, 0.1f);
                radiance = vec3(0.6f, 0.6f, 0.6f);
            } else {
                radiance *= stack[i].transmission;
                radiance += stack[i].light;
                if (i == 0) {
                    // to make light sources visible: direction illumination into camera
                    // clamp to [0.0, 1.0] for anti-aliasing
                    radiance += min(stack[i].emission, 1.0f);
                }
            }
        }

        if (stack[0].isSky) {
//            radiance = vec3(0.0f);
//            radiance = vec3(0.7f, 0.75f, 1.0f);
        } else {
//            int t = 1;
//            if (!stack[t].isSky) {
//                radiance = stack[t].debug;
//            } else {
//                radiance = vec3(0.0f);
//            }
        }
        if (isfinite(radiance.x) && isfinite(radiance.y) && isfinite(radiance.z)) {
            sum += radiance;
        }
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
