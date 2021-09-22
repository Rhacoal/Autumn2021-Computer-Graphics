#include "common.glsl"

#ifdef VERTEX_UV
in vec2 vUv;
#endif
#ifdef VERTEX_COLOR
in vec4 vColor;
#endif

#ifdef COLOR_MAP
uniform sampler2D colorMap;
#endif
#ifdef NORMAL_MAP
uniform sampler2D normalMap;
#endif

uniform mat4 modelViewMatrix;
uniform mat4 projectionMatrix;

#ifdef DIRECTIONAL_LIGHT
uniform vec4 dirLightPosition;
uniform vec4 dirLightColor;
uniform float dirLightRange;
#endif
#ifdef defined(POINT_LIGHT) && defined(POINT_NUM)
uniform vec4 pointLightPositions[POINT_NUM];
uniform vec4 pointLightColors[POINT_NUM];
#endif

out vec4 color;

void main() {
    mat4 mvpMatrix = modelViewMatrix * projectionMatrix;

    #ifdef NORMAL_MAP
    vec3 normal = (mvpMatrix * vec4(0.0, 1.0, 0.0, 0.0)).xyz;
    #else
    vec3 normal = vec4(0.0, 1.0, 0.0, 1.0).xyz;
    #endif

    float alpha;
    vec3 diffuseColor;

    #ifdef COLOR_MAP
    vec4 texColor = texture2D(colorMap, vUv);
    diffuseColor = texColor.rgb;
    alpha = texColor.a;
    #else
    alpha = 1.0;
    #endif


    #ifdef DIRECTIONAL_LIGHT

    #endif

    #if defined(POINT_LIGHT) && defined(POINT_NUM)
    // point light
    #endif

    color = vec4(1.0, 0.0, 0.0, alpha);
}
