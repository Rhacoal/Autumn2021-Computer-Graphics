#ifndef ASSIGNMENT_CG_COMMON_H
#define ASSIGNMENT_CG_COMMON_H

#define GLFW_INCLUDE_NONE
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

namespace cg {
inline int initGL() {
    return gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
}

namespace math {
constexpr double pi() {
    return 3.14159265358979323846;
}

constexpr double half_pi() {
    return 1.57079632679489661923;
}
}

}
#endif //ASSIGNMENT_CG_COMMON_H
