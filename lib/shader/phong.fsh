#version 330 core

// {headers}

struct PointLight {
    vec3 direction;
    vec3 position;
    vec4 color;
};

struct DirectionalLight {
    vec3 direction;
    vec4 color;
};

#if POINT_LIGHT_COUNT
uniform PointLight pointLights[POINT_LIGHT_COUNT];
#endif

#if DIRECTIONAL
uniform DirectionalLight directionalLight;
#endif

uniform mat4 modelMatrix, mvpMatrix;
uniform vec3 cameraPosition;

uniform sampler2D diffuse;
uniform float shininess;
uniform vec4 color;


in vec4 vColor;
in vec2 vUv;
in vec3 worldPosition;

out vec4 fragColor;

void main() {
    vec4 diffuseColor = texture(diffuse, vUv);
//    fragColor = vec4((vPos + 1) * 0.5, 1.0);
    float opacity = diffuseColor.a;

    for (int i = 0; i < POINT_LIGHT_COUNT; ++i) {
        vec3 L = normalize(pointLights[i].position - worldPosition);
        vec3 V = normalize(camera.position - worldPosition);
        vec3 H = normalize(L + V);

    }
    fragColor = vec4(1.0);
}
