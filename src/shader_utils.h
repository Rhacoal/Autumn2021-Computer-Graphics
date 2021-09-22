#ifndef ASSIGNMENT_SHADER_UTILS_H
#define ASSIGNMENT_SHADER_UTILS_H

#include <string>
#include <map>

enum class uniform {
    FLOAT, INTEGER, VEC4, VEC3,
};

static std::map<std::string, std::map<std::string, uniform>> uniforms = {
    {"Phong", std::map<std::string, uniform>{
        {"PointLightCount",       uniform::INTEGER},
        {"DirectionalLightColor", uniform::VEC4},
        {"DirectionalLightDir",   uniform::VEC3},
    }},
};

#endif //ASSIGNMENT_SHADER_UTILS_H
