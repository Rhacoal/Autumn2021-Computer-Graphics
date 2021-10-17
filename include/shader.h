#ifndef ASSIGNMENT_SHADER_H
#define ASSIGNMENT_SHADER_H

#include <cg_common.h>
#include <cg_fwd.h>

#include <string>

namespace cg {
class Shader {
public:
    GLuint id;

    Shader();

    /**
     * Constructs a shader object using shader source.
     * @param vertexShaderSource
     * @param fragmentShaderSource
     */
    Shader(const char *vertexShaderSource, const char *fragmentShaderSource);

    Shader(const Shader &) = delete;

    Shader(Shader &&) noexcept;

    Shader &operator=(Shader &&shader) noexcept;

    void use() const;

    ~Shader();
};
}

#endif //ASSIGNMENT_SHADER_H
