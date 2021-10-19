#version 330 core

// {headers}

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;
layout (location = 2) in vec2 uv;
layout (location = 3) in vec3 normal;

uniform mat4 modelMatrix;
uniform mat4 mvpMatrix;

out vec4 vColor;
out vec2 vUv;
out vec3 vNormal;
out vec3 worldPosition;

void main() {
    vColor = vec4((position + 1.0) / 2, 1.0);
    vUv = uv;

    vec4 pos = mvpMatrix * vec4(position, 1.0);
    vNormal = (modelMatrix * vec4(normal, 0.0)).xyz;
    worldPosition = pos.xyz / pos.w;

    gl_Position = pos;
}
