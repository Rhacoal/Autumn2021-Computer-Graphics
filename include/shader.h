#ifndef ASSIGNMENT_SHADER_H
#define ASSIGNMENT_SHADER_H

#include <cg_common.h>
#include <cg_fwd.h>

#include <string>
#include <vector>

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

    void setUniform4f(const char *name, const glm::vec4 &value) {
        glUniform4fv(glGetUniformLocation(id, name), 1, &value[0]);
    }

    void setUniform3f(const char *name, const glm::vec3 &value) {
        glUniform3fv(glGetUniformLocation(id, name), 1, &value[0]);
    }

    void setUniform1f(const char *name, float value) {
        glUniform1f(glGetUniformLocation(id, name), value);
    }

    void setUniform1i(const char *name, int value) {
        glUniform1i(glGetUniformLocation(id, name), value);
    }

    void setUniformMatrix4(const char *name, const glm::mat4 &value, bool transpose = false) {
        glUniformMatrix4fv(glGetUniformLocation(id, name), 1, transpose, &value[0][0]);
    }

    ~Shader();
};

struct ShaderPassImpl;

class ShaderPass {
    std::shared_ptr<ShaderPassImpl> _impl;
public:
    ShaderPass(const char *fragmentShaderSource, int width, int height);

    ShaderPass(ShaderPass &&) = default;

    ShaderPass(const ShaderPass &) = default;

    void resize(int width, int height);

    void renderBegin();

    void renderEnd();

    void thenBegin(ShaderPass &next);
};

class ShaderPassLink {
    int _width, _height;
    std::vector<ShaderPass*> passes;
public:
    ShaderPassLink(int width, int height);

    template<typename Iter>
    void usePasses(Iter first, Iter last) {
        passes.clear();
        passes.insert(passes.end(), first, last);
    }

    void renderBegin();

    void renderEnd();

    void resize(int width, int height);
};
}

#endif //ASSIGNMENT_SHADER_H
