#ifndef ASSIGNMENT_BXDF_H
#define ASSIGNMENT_BXDF_H

#include "lib/shaders/rt_definition.h"

float sqr(float x) { return x * x; }

float SchlickFresnel(float u) {
    float m = clamp(1 - u, 0, 1);
    float m2 = m * m;
    return m2 * m2 * m; // pow(m,5)
}

float GTR1(float NdotH, float a) {
    if (a >= 1) return 1 / RT_M_PI_F;
    float a2 = a * a;
    float t = 1 + (a2 - 1) * NdotH * NdotH;
    return (a2 - 1) / (RT_M_PI_F * log(a2) * t);
}

float GTR2(float NdotH, float a) {
    float a2 = a * a;
    float t = 1 + (a2 - 1) * NdotH * NdotH;
    return a2 / (RT_M_PI_F * t * t);
}

float GTR2_aniso(float NdotH, float HdotX, float HdotY, float ax, float ay) {
    return 1 / (RT_M_PI_F * ax * ay * sqr(sqr(HdotX / ax) + sqr(HdotY / ay) + NdotH * NdotH));
}

float smithG_GGX(float NdotV, float alphaG) {
    float a = alphaG * alphaG;
    float b = NdotV * NdotV;
    return 1.0f / (NdotV + sqrt(a + b - a * b));
}

float smithG_GGX_aniso(float NdotV, float VdotX, float VdotY, float ax, float ay) {
    return 1.0f / (NdotV + sqrt(sqr(VdotX * ax) + sqr(VdotY * ay) + sqr(NdotV)));
}

float3 mon2lin(float3 x) {
    return vec3(pow(x.x, 2.2f), pow(x.y, 2.2f), pow(x.z, 2.2f));
}

float3 BRDF(float3 L, float3 V, float3 N, float3 X, float3 Y,
            float3 baseColor, float subsurface, float metallic,
            float specular, float3 specularTint, float roughness, float anisotropic,
            float sheen, float3 sheenTint, float clearcoat, float clearcoatGloss) {
    float NdotL = dot(N, L);
    float NdotV = dot(N, V);
    if (NdotL < 0 || NdotV < 0) return vec3(0.0f);

    float3 H = normalize(L + V);
    float NdotH = dot(N, H);
    float LdotH = dot(L, H);

    float luminance = .3f * baseColor.x + .6f * baseColor.y + .1f * baseColor.z; // luminance

    float3 C_tint = luminance > 0 ? baseColor / luminance : vec3(1.0f); // normalize lum. to isolate hue+sat
    float3 C_spec0 = mix(specular * .08f * mix(vec3(1.0f), C_tint, specularTint), baseColor, metallic);
    float3 C_sheen = mix(vec3(1.0f), C_tint, sheenTint);

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
    float aspect = sqrt(1 - anisotropic * .9f);
    float ax = max(.001f, sqr(roughness) / aspect);
    float ay = max(.001f, sqr(roughness) * aspect);
    float Ds = GTR2_aniso(NdotH, dot(H, X), dot(H, Y), ax, ay);
    float FH = SchlickFresnel(LdotH);
    float3 Fs = mix(C_spec0, vec3(1.0f), FH);
    float Gs;
    Gs = smithG_GGX_aniso(NdotL, dot(L, X), dot(L, Y), ax, ay);
    Gs *= smithG_GGX_aniso(NdotV, dot(V, X), dot(V, Y), ax, ay);

    // sheen
    float3 Fsheen = FH * sheen * C_sheen;

    // clearcoat (ior = 1.5 -> F0 = 0.04)
    float Dr = GTR1(NdotH, mix(.1f, .001f, clearcoatGloss));
    float Fr = mix(.04f, 1.0f, FH);
    float Gr = smithG_GGX(NdotL, .25f) * smithG_GGX(NdotV, .25f);

    return (RT_M_1_PI * mix(Fd, ss, subsurface) * baseColor + Fsheen)
           * (1 - metallic)
           + Gs * Fs * Ds + .25f * clearcoat * Gr * Fr * Dr;
}

#endif //ASSIGNMENT_BXDF_H
