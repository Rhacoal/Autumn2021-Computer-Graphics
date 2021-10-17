#version 330 core

// {headers}

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;
layout (location = 2) in vec2 uv;

uniform mat4 modelMatrix;
uniform mat4 mvpMatrix;

out vec4 vColor;
out vec2 vUv;
out vec3 worldPosition;

void main() {
    vColor = vec4(color, 1.0);
    vUv = uv;

    vec4 pos = mvpMatrix * vec4(position, 1.0);
    pos /= pos.w;
    worldPosition = position.xyz;

    gl_Position = pos;
}
