#version 330 core

layout(location = 0) in vec3 position;
#ifdef VERTEX_UV
layout(location = 1) in vec2 vertex_uv;
#endif VERTEX_UV
#ifdef VERTEX_COLOR
layout(location = 2) in vec4 vertex_color;
#endif

#ifdef VERTEX_UV
out vec2 vUv;
#endif
#ifdef VERTEX_COLOR
out vec4 vColor;
#endif
uniform mat4 modelViewMatrix;
uniform mat4 projectionMatrix;

void main() {
    vUv = vertex_uv;
    vColor = vertex_color;
    gl_Position = modelViewMatrix * projectionMatrix * vec4(position, 1.0);
}