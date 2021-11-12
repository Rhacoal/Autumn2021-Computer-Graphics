#include <shader.h>
#include <texture.h>

namespace cg {
static std::string getShaderSource(GLuint shader) {
    constexpr size_t buf_size = 1u << 20u;
    static char buf[buf_size];
    glGetShaderSource(shader, buf_size, nullptr, buf);
    return buf;
}

struct ShaderImpl {
    GLuint id;

    ShaderImpl() noexcept: id(0) {}

    ShaderImpl(const char *vertexShaderSource, const char *fragmentShaderSource) {
        // compile vertex shader
        GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
        glCompileShader(vertexShader);

        // check for compilation errors
        int success;
        char infoLog[512];
        glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
            fprintf(stderr, "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n%s\n", infoLog);
        }

        // compile fragment shader
        GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
        glCompileShader(fragmentShader);
        // check for compilation errors
        glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(fragmentShader, 511, nullptr, infoLog);
            fprintf(stderr, "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n%s\n", infoLog);
        }

        // link program
        GLuint shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vertexShader);
        glAttachShader(shaderProgram, fragmentShader);
        glLinkProgram(shaderProgram);

        // check for linking errors
        glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shaderProgram, 511, nullptr, infoLog);
            fprintf(stderr, "ERROR::SHADER::PROGRAM::LINKING_FAILED\n%s\n", infoLog);
            auto vs = getShaderSource(vertexShader);
            auto fs = getShaderSource(fragmentShader);
            fprintf(stderr, "Vertex Shader:\n%s\n==========\nFragment Shader:\n%s\n\n", vs.c_str(), fs.c_str());
            exit(0);
        }
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        id = shaderProgram;
    }

    ShaderImpl(ShaderImpl &&b) noexcept: id(b.id) {
        b.id = 0;
    }

    GLuint use() const {
        glUseProgram(id);
        return id;
    }

    bool isNull() const {
        return id == 0;
    }

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

    ~ShaderImpl() {
        if (id) {
            glDeleteProgram(id);
        }
    }
};

Shader::Shader() : _impl(std::make_shared<ShaderImpl>()) {}

GLuint Shader::use() const {
    return _impl->use();
}

bool Shader::isNull() const noexcept {
    return _impl->isNull();
}

Shader::Shader(const char *vertexShaderSource, const char *fragmentShaderSource)
    : _impl(std::make_shared<ShaderImpl>(vertexShaderSource, fragmentShaderSource)) {
}

void Shader::setUniform4f(const char *name, const glm::vec4 &value) {
    _impl->setUniform4f(name, value);
}

void Shader::setUniform3f(const char *name, const glm::vec3 &value) {
    _impl->setUniform3f(name, value);
}

void Shader::setUniform1f(const char *name, float value) {
    _impl->setUniform1f(name, value);
}

void Shader::setUniform1i(const char *name, int value) {
    _impl->setUniform1i(name, value);
}

void Shader::setUniformMatrix4(const char *name, const glm::mat4 &value, bool transpose) {
    _impl->setUniformMatrix4(name, value, transpose);
}

GLuint Shader::id() const {
    return _impl->id;
}

static constexpr const char *shaderPassVertexShader = R"(
#version 330 core

layout (location = 0) in vec2 inTexCoord;
out vec2 texCoord;

uniform sampler2D screenTexture;

void main() {
    texCoord = inTexCoord;
    gl_Position = vec4(inTexCoord * 2.0 - 1.0, 0.0, 1.0);
}
)";

struct ShaderPassImpl {
    static constexpr float vertices[] = {0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0, 0.0};
    static constexpr unsigned short indices[] = {0, 1, 2, 0, 2, 3};

    Shader shader;
    GLuint vao{}, vbo{}, ebo{};
    GLuint fbo{}, rbo{};
    Texture colorBuffer;
    int buffer_width, buffer_height;

    ShaderPassImpl(const char *fragmentShaderSource, int width, int height)
        : shader(shaderPassVertexShader, fragmentShaderSource), buffer_width(width), buffer_height(height) {
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, nullptr);
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        GLuint id = shader.use();
        glUniform1i(glGetUniformLocation(id, "screenTexture"), 0);
        createFrameBuffer(width, height);
    }

    void deleteFrameBuffer() {
        if (fbo) {
            glDeleteFramebuffers(1, &fbo);
            glDeleteRenderbuffers(1, &rbo);
        }
        fbo = 0u;
    }

    void createFrameBuffer(int width, int height) {
        glGenFramebuffers(1, &fbo);

        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        // create a color attachment texture
        colorBuffer.generate(width, height, GL_RGB32F);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorBuffer.tex(), 0);
        // create a renderbuffer object for depth and stencil attachment (we won't be sampling these)
        glGenRenderbuffers(1, &rbo);
        glBindRenderbuffer(GL_RENDERBUFFER, rbo);
        // use a single renderbuffer object for both a depth AND stencil buffer.
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);
        // now that we actually created the framebuffer and added all attachments we want to check if it is actually complete now
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            fputs("ERROR::FRAMEBUFFER:: Framebuffer is not complete!\n", stderr);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void resize(int width, int height) {
        if (width == this->buffer_width && height == this->buffer_height) {
            return;
        }
        this->buffer_width = width;
        this->buffer_height = height;
        deleteFrameBuffer();
        createFrameBuffer(width, height);
    }

    void renderBegin() {
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glEnable(GL_DEPTH_TEST);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void renderEnd() {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDisable(GL_DEPTH_TEST);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        shader.use();
        glBindVertexArray(vao);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, colorBuffer.tex());
        glDrawElements(GL_TRIANGLES, util::arraySize(indices), GL_UNSIGNED_SHORT, nullptr);
        glBindVertexArray(0);
    }

    void thenBegin(ShaderPassImpl &impl) {
        impl.renderBegin();
        glDisable(GL_DEPTH_TEST);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        shader.use();
        glBindVertexArray(vao);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, colorBuffer.tex());
        glDrawElements(GL_TRIANGLES, util::arraySize(indices), GL_UNSIGNED_SHORT, nullptr);
        glBindVertexArray(0);
    }

    ~ShaderPassImpl() {
        glDeleteVertexArrays(1, &vao);
        glDeleteBuffers(1, &vbo);
        glDeleteBuffers(1, &ebo);
        deleteFrameBuffer();
    }
};

ShaderPassLink::ShaderPassLink(int width, int height) : _width(width), _height(height) {
}

void ShaderPassLink::renderBegin() {
    for (auto &pass: _passes) {
        pass->resize(_width, _height);
    }
    if (!_passes.empty()) {
        _passes[0]->renderBegin();
    }
}

void ShaderPassLink::renderEnd() {
    for (int i = 0; i + 1 < _passes.size(); ++i) {
        _passes[i]->thenBegin(*_passes[i + 1]);
    }
    if (!_passes.empty()) {
        _passes.back()->renderEnd();
    }
}

void ShaderPassLink::resize(int width, int height) {
    _width = width;
    _height = height;
}

ShaderPass::ShaderPass(const char *fragmentShaderSource, int width, int height)
    : _impl(std::make_shared<ShaderPassImpl>(fragmentShaderSource, width, height)) {
}

void ShaderPass::resize(int width, int height) {
    _impl->resize(width, height);
}

void ShaderPass::renderBegin() {
    _impl->renderBegin();
}

void ShaderPass::renderEnd() {
    _impl->renderEnd();
}

void ShaderPass::thenBegin(ShaderPass &next) {
    _impl->thenBegin(*next._impl);
}
}
