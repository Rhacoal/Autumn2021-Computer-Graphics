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

uniform sampler2D diffuse;
uniform float shininess;
uniform vec4 color;

in vec4 vColor;
in vec2 vUv;

out vec4 fragColor;

void main() {
    vec4 result = texture(diffuse, vUv);
    fragColor = result * color;
}
