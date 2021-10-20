#version 330 core

// {headers}

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;
layout (location = 2) in vec2 uv;
layout (location = 3) in vec3 normal;

#if USE_INSTANCE
uniform mat4 modelMatrices[INSTANCE_COUNT];
uniform mat4 mvpMatrices[INSTANCE_COUNT];
#else
uniform mat4 modelMatrix;
uniform mat4 mvpMatrix;
#endif

out vec4 vColor;
out vec2 vUv;
out vec3 vNormal;
out vec3 worldPosition;

void main() {
    vColor = vec4((position + 1.0) / 2, 1.0);
    vUv = uv;

#if USE_INSTANCE
    mat4 mvpMatrix = mvpMatrices[gl_InstanceID];
    mat4 modelMatrix = mvpMatrices[gl_InstanceID];
#endif
    vec4 pos = mvpMatrix * vec4(position, 1.0);
    vNormal = (modelMatrix * vec4(normal, 0.0)).xyz;
    vec4 wPos = modelMatrix * vec4(position, 1.0);
    worldPosition = wPos.xyz;

    gl_Position = pos;
}
