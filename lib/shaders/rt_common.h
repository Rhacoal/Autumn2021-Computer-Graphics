#ifndef ASSIGNMENT_RT_COMMON_H
#define ASSIGNMENT_RT_COMMON_H

#ifdef __cplusplus

#include <simcl.h>

#define CPP_INLINE inline
#define debugger do {int a = 0;}while(0)
#else
#define CPP_INLINE
#define debugger
#define vec3 (float3)
#define vec4 (float4)
#endif

#define RT_M_PI_F           3.14159274101257f
#define RT_M_PI             3.141592653589793115998
#define RT_M_1_PI_F         0.31830987334251f
#define RT_M_1_PI           0.318309886183790691216

uint next(ulong *, int);

float randomFloat(ulong *);

float pow2(float v);

CPP_INLINE uint next(ulong *seed, int bits) {
    *seed = (*seed * 0x5DEECE66DL + 0xBL) & (((ulong) 1ul << 48) - 1);
    return (uint) (*seed >> (48 - bits));
}

CPP_INLINE float randomFloat(ulong *seed) {
    return (float) next(seed, 24) / (float) (1 << 24);
}

CPP_INLINE uint randomInt(ulong *seed, uint upper) {
    return next(seed, 32) % upper;
}

CPP_INLINE float pow2(float v) {
    return v * v;
}

CPP_INLINE uint pow3i(uint p) {
    return p * p * p;
}

CPP_INLINE float pow5(float v) {
    float t = v * v;
    return t * t * v;
}

CPP_INLINE float3 reflect(float3 I, float3 N) {
    return -I + 2.0f * N * dot(N, I);
}

CPP_INLINE float3 cosineWeightedHemisphereSample(float3 normal, float *pdf, ulong *seed) {
    float a = randomFloat(seed), b = randomFloat(seed);
    float phi = RT_M_PI_F * 2.0f * a, r = sqrt(b);
    float x = r * cos(phi), z = r * sin(phi);
    float y = sqrt(1 - b);

    // cosine-weighted hemisphere sampling
    float3 temp = fabs(normal.x) > 0.1f ? vec3(0.0f, 1.0f, 0.0f) : vec3(1.0f, 0.0f, 0.0f);
    *pdf = y * RT_M_1_PI_F;
    float3 u = normalize(cross(temp, normal));
    float3 v = cross(normal, u);

    return u * x + normal * y + v * z;
}

CPP_INLINE float3 metallicBRDFSample(float3 normal, float3 wo, float roughness, float *pdf, ulong *seed) {
    if (roughness < 1e-5) {
        // perfect reflection
        // need divide by distance
        *pdf = 1.0f;
        return reflect(wo, normal);
    }
    return cosineWeightedHemisphereSample(normal, pdf, seed);
}

CPP_INLINE float3 surfaceGGXSample(float3 normal, float3 wo, float roughness, float eta, float *pdf, ulong *seed) {
    
}

#endif //ASSIGNMENT_RT_COMMON_H
