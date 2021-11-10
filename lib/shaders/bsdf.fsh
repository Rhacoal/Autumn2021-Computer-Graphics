#define RT_M_PI_F           3.14159274101257f
#define RT_M_1_PI_F         0.31830987334251f

float sqr(float x) {
    return x * x;
}

float SchlickFresnel(float u) {
    float m = clamp(1.0f - u, 0.0f, 1.0f);
    float m2 = m * m;
    return m2 * m2 * m;
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

vec3 DisneyBRDF(vec3 L, vec3 V, vec3 N, vec3 baseColor, float subsurface, float metallic, float specular, float specularTint, float roughness) {
    float NdotL = dot(N, L);
    float NdotV = dot(N, V);
    if (NdotL < 0 || NdotV < 0) return vec3(0.0f);

    vec3 H = normalize(L + V);
    float NdotH = dot(N, H);
    float LdotH = dot(L, H);

    float luminance = .3f * baseColor.x + .6f * baseColor.y + .1f * baseColor.z;

    vec3 cTint = luminance > 0 ? baseColor / luminance : vec3(1.0f);
    vec3 colorSpecular = mix(specular * .08f * mix(vec3(1.0f), cTint, specularTint), baseColor, metallic);

    // Diffuse fresnel - go from 1 at normal incidence to .5 at grazing and mix in diffuse retro-reflection based on roughness
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
    vec3 Fs = mix(colorSpecular, vec3(1.0f), FH);
    float Gs = smithG_GGX(NdotV, a);

    return (RT_M_1_PI_F * mix(Fd, ss, subsurface) * baseColor) * (1 - metallic) + Gs * Fs * Ds;
}
