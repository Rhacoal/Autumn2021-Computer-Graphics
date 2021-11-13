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

float DielectricFresnel(float VdotH, float eta) {
    // use 1.0f as R90
    float sinVH = 1.0f - pow2(VdotH);
    float sinLH2 = eta * eta * sinVH;
    if (sinLH2 >= 1) {
        return 1.0f;
    }
    float cosLH = sqrt(1 - sinLH2);
    float R0 = pow2((eta - 1.0f) / (eta + 1.0f));
    return mix(R0, 1.0f, pow5(1.0f - cosLH));
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

float luminance(float3 color) {
    return color.x * .6f + color.y * .3f + color.z * .1f;
}

float3 DiffuseBRDF(float3 L, float3 V, float3 N, float roughness, float3 diffuseColor, float eta) {
    float NdotL = dot(N, L);
    float NdotV = dot(N, V);
    if (NdotV <= 0.0f || NdotL <= 0.0f) return vec3(0.0f);
    float3 H = normalize(L + V);
    float LdotH = dot(L, H);
    // diffuse fresnel
    float FL = SchlickFresnel(NdotL), FV = SchlickFresnel(NdotV);
    float Fd90 = 0.5f + 2 * LdotH * LdotH * roughness;
    float Fd = mix(1.0f, Fd90, FL) * mix(1.0f, Fd90, FV);
//    float Rtheta = DielectricFresnel(NdotL, eta);
    return RT_M_1_PI_F * Fd * diffuseColor;
}

float3 MetallicBRDF(float3 L, float3 V, float3 N, float roughness, float3 specularF0) {
    float NdotV = dot(N, V);
    float NdotL = dot(N, L);
    if (NdotV <= 0.0f || NdotL <= 0.0f) return vec3(0.0f);
    float3 H = normalize(L + V);
    float LdotH = dot(L, H);
    float NdotH = dot(N, H);
    float a = max(.001f, sqr(roughness));
    float Ds = GTR2(NdotH, a);
    float FH = SchlickFresnel(LdotH);
    float3 Fs = mix(specularF0, vec3(1.0f), FH);
    float Gs = smithG_GGX(NdotV, a);
    return clamp(Gs * Fs * Ds, 0.0f, 1.0f);
}

float3 BRDF(float3 L, float3 V, float3 N, float roughness, float3 diffuseColor, float3 specularF0, float eta) {
    return DiffuseBRDF(L, V, N, roughness, diffuseColor, eta) + MetallicBRDF(L, V, N, roughness, specularF0);
}

#endif //ASSIGNMENT_BXDF_H
