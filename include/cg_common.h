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
#include <stdexcept>
#include <cstdio>
#include <fstream>
#include <filesystem>

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
        throw std::runtime_error(buf);
    }
}

namespace math {
template<typename F = double>
constexpr F pi() {
    return F(3.14159265358979323846);
}

constexpr float radians(float degree) {
    return degree / 180.f * pi<float>();
}

constexpr double radians(double degree) {
    return degree / 180. * pi<double>();
}

template<typename F>
constexpr double radians(F degree) {
    return radians(double(degree));
}

constexpr double half_pi() {
    return 1.57079632679489661923;
}
}

namespace util {
template<typename T, size_t N>
constexpr size_t arraySize(T (&arr)[N]) {
    return N;
}

inline void printMatrix(const glm::mat4 &mat) {
    puts("[");
    for (int i = 0; i < 4; ++i) {
        printf("%f %f %f %f\n", mat[i][0], mat[i][1], mat[i][2], mat[i][3]);
    }
    puts("]");
}

template<typename T>
inline void printVec(T t) {
    if (t.length() >= 3) {
        printf("%f %f %f", t[0], t[1], t[2]);
    }
    if (t.length() >= 4) {
        printf(" %f", t[3]);
    }
    puts("");
}
}

inline std::string readFile(const char *path) {
    auto fspath = std::filesystem::u8path(path);
    std::ifstream in(fspath, std::ios::in | std::ios::binary);
    return std::string{std::istreambuf_iterator<char>(in), {}};
}

inline std::string readFile(const std::filesystem::path &fspath) {
    std::ifstream in(fspath, std::ios::in | std::ios::binary);
    return std::string{std::istreambuf_iterator<char>(in), {}};
}
}
#endif //ASSIGNMENT_CG_COMMON_H
