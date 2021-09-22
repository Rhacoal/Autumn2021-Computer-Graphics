#version 330 core

// {headers}

uniform vec4 PointLightDirs[PointLightCount];
uniform vec4 PointLightColors[PointLightCount];
uniform vec4 PointLightPositions[PointLightCount];

uniform vec4 DirectionLightColor;
uniform vec3 DirectionLightDir;

uniform float test[2][3];
