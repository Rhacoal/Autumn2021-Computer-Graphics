#version 330 core

// {headers}

layout (location = 0) in vec3 position;
layout (location = 1) in vec2 uv;
layout (location = 2) in vec3 normal;

uniform mat4 modelMatrix;
uniform mat4 mvpMatrix;

out vec2 vUv;
out vec3 vNormal;
out vec3 worldPosition;

void main() {
    vUv = uv;

    vec4 pos = mvpMatrix * vec4(position, 1.0);
    vNormal = (modelMatrix * vec4(normal, 0.0)).xyz;
    vec4 wPos = modelMatrix * vec4(position, 1.0);
    worldPosition = wPos.xyz;

    gl_Position = pos;
}
