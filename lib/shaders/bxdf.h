// Copyright Disney Enterprises, Inc.  All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License
// and the following modification to it: Section 6 Trademarks.
// deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the
// trade names, trademarks, service marks, or product names of the
// Licensor and its affiliates, except as required for reproducing
// the content of the NOTICE file.
//
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#ifndef ASSIGNMENT_BXDF_H
#define ASSIGNMENT_BXDF_H

#include "lib/shaders/rt_definition.h"

float sqr(float x) { return x * x; }

float SchlickFresnel(float u) {
    float m = clamp(1.0f - u, 0.0f, 1.0f);
    float m2 = m * m;
    return m2 * m2 * m; // pow(m,5)
}

float GTR1(float NdotH, float a) {
    if (a >= 1) return 1.0f / RT_M_PI_F;
    float a2 = a * a;
    float t = 1.0f + (a2 - 1.0f) * NdotH * NdotH;
    return (a2 - 1.0f) / (RT_M_PI_F * log(a2) * t);
}

float GTR2(float NdotH, float a) {
    float a2 = a * a;
    float t = 1.0f + (a2 - 1.0f) * NdotH * NdotH;
    return a2 / (RT_M_PI_F * t * t);
}

float smithG_GGX(float NdotV, float alphaG) {
    float a = alphaG * alphaG;
    float b = NdotV * NdotV;
    return 1.0f / (NdotV + sqrt(a + b - a * b));
}

float3 Specular_BSDF_Transmission(float3 L, float3 V, float3 N, float ior,
                                  float3 baseColor, float metallic, float specular, float specularTint,
                                  float roughness) {
    float3 ht = -normalize(L + V * ior);
    return ht;
    // http://www.cs.cornell.edu/courses/cs6630/2009fa/slides/egsr07-bsdf.pdf
//    smithG_GGX()
}

float3 Disney_BRDF(float3 L, float3 V, float3 N,
            float3 baseColor, float subsurface, float metallic,
            float specular, float specularTint, float roughness) {
    float NdotL = dot(N, L);
    float NdotV = dot(N, V);
    if (NdotL < 0 || NdotV < 0) return vec3(0.0f);

    float3 H = normalize(L + V);
    float NdotH = dot(N, H);
    float LdotH = dot(L, H);

    float luminance = .3f * baseColor.x + .6f * baseColor.y + .1f * baseColor.z; // luminance

    float3 C_tint = luminance > 0 ? baseColor / luminance : vec3(1.0f); // normalize lum. to isolate hue+sat
    float3 C_spec0 = mix(specular * .08f * mix(vec3(1.0f), C_tint, specularTint), baseColor, metallic);

    // Diffuse fresnel - go from 1 at normal incidence to .5 at grazing
    // and mix in diffuse retro-reflection based on roughness
    float FL = SchlickFresnel(NdotL), FV = SchlickFresnel(NdotV);
    float Fd90 = 0.5f + 2 * LdotH * LdotH * roughness;
    float Fd = mix(1.0f, Fd90, FL) * mix(1.0f, Fd90, FV);

    // Based on Hanrahan-Krueger brdf approximation of isotropic bssrdf
    // 1.25 scale is used to (roughly) preserve albedo
    // Fss90 used to "flatten" retroreflection based on roughness
    float Fss90 = LdotH * LdotH * roughness;
    float Fss = mix(1.0f, Fss90, FL) * mix(1.0f, Fss90, FV);
    float ss = 1.25f * (Fss * (1 / (NdotL + NdotV) - .5f) + .5f);

    // specular
    float a = max(.001f, sqr(roughness));
    float Ds = GTR2(NdotH, a);
    float FH = SchlickFresnel(LdotH);
    float3 Fs = mix(C_spec0, vec3(1.0f), FH);
    float Gs = smithG_GGX(NdotV, a);

    return (RT_M_1_PI_F * mix(Fd, ss, subsurface) * baseColor) * (1 - metallic) + Gs * Fs * Ds;
}

#endif //ASSIGNMENT_BXDF_H
