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

uniform vec4 ambientLight;

uniform mat4 modelMatrix, mvpMatrix;
uniform vec3 cameraPosition;

uniform sampler2D diffuse;
uniform float shininess;
uniform vec4 color;


in vec4 vColor;
in vec2 vUv;
in vec3 worldPosition;
in vec3 vNormal;

out vec4 fragColor;

void main() {
    vec3 result = vec3(0.0);

    vec4 diffuseColor = texture(diffuse, vUv);
    float opacity = diffuseColor.a;

#if DIRECTIONAL
    vec3 L = normalize(directionalLight.direction);
    vec3 V = normalize(cameraPosition - worldPosition);
    vec3 H = normalize(L + V);
    vec3 N = normalize(vNormal);

    float spec = pow(clamp(dot(N, H), 0.0, 1.0), shininess);
    result += (spec * directionalLight.color.rgb * directionalLight.color.a);
#endif
    result.rgb += ambientLight.rgb * ambientLight.a;
    result *= color.rgb;

    fragColor = vec4(result, 1.0);
}
