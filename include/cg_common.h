#ifndef ASSIGNMENT_CG_COMMON_H
#define ASSIGNMENT_CG_COMMON_H

#define check_err(x) do{::cg::flush_err();x;::cg::assert_ok();}while(0)

#define GLFW_INCLUDE_NONE
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#if defined(_MSC_VER)
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#endif

#include <cstdint>
#include <exception>

namespace cg {
inline int initGL() {
    return gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
}

inline void flush_err() {
    while (glGetError());
}

inline void assert_ok() {
    static char buf[256];
    auto err = glGetError();
    if (err) {
        snprintf(buf, 256, "GLError: %d\n", err);
        throw std::exception(buf);
    }
}

void APIENTRY glDebugOutput(GLenum source, GLenum type, unsigned int id, GLenum severity,
                            GLsizei length, const char *message, const void *userParam);

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
