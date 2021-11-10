#version 330 core
#include <bsdf.fsh>

// {headers}
#define RECIPROCAL_PI 0.3183098861837907

struct PointLight {
    vec3 direction;
    vec3 position;
    vec3 color;
};

struct DirectionalLight {
    vec3 direction;
    vec3 color;
};

#if POINT_LIGHT_COUNT
uniform PointLight pointLights[POINT_LIGHT_COUNT];
#endif

#if DIRECTIONAL
uniform DirectionalLight directionalLight;
#endif

uniform vec3 ambientLight;

uniform mat4 modelMatrix, mvpMatrix;
uniform vec3 cameraPosition;

uniform sampler2D albedoMap;
uniform vec4 color;
uniform sampler2D metallicMap;
uniform float metallicIntensity;
uniform sampler2D roughnessMap;
uniform float roughnessIntensity;
uniform sampler2D aoMap;
uniform float aoIntensity;
uniform sampler2D normalMap;

in vec2 vUv;
in vec3 worldPosition;
in vec3 vNormal;

out vec4 fragColor;

float rand(vec2 co){
    return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

void main() {
    vec3 result = vec3(0.0);

    vec4 albedoColor = texture(albedoMap, vUv);
    vec3 baseColor = albedoColor.rgb * color.rgb;
    float opacity = albedoColor.a * color.a;
    float metallic = texture(metallicMap, vUv).r * metallicIntensity;
    float roughness = texture(roughnessMap, vUv).g * roughnessIntensity;
    float ao = texture(aoMap, vUv).b * aoIntensity;

    vec3 V = normalize(cameraPosition - worldPosition);
    vec3 N = normalize(vNormal);

#if DIRECTIONAL
    {
        vec3 L = normalize(directionalLight.direction);
        vec3 H = normalize(L + V);
        vec3 brdf = DisneyBRDF(L, V, N, baseColor.rgb, 0.0f, metallic, 0.5f, 0.0f, roughness);

        result += directionalLight.color * brdf;
    }
#endif
#if POINT_LIGHT_COUNT
    for (int i = 0; i < POINT_LIGHT_COUNT; ++i) {
        vec3 lightVec = pointLights[i].position - worldPosition;
        vec3 L = normalize(directionalLight.direction);
        vec3 H = normalize(L + V);
        vec3 brdf = DisneyBRDF(L, V, N, baseColor.rgb, 0.0f, metallic, 0.5f, 0.0f, roughness);

        result += pointLights[i].color * brdf / length(lightVec);
    }
#endif
    result.rgb += ambientLight * mix(vec3(0.04f), baseColor.rgb, metallic);

    fragColor = vec4(result, opacity);
}
