#include <shader.h>
#include <texture.h>

std::string readFile(const char *path) {
    std::string ret;
    char buf[1024];
    FILE *f = fopen(path, "rb");
    int n;
    while ((n = fread(buf, 1, 1024, f))) {
        ret.insert(ret.back(), buf, n);
    }
    return ret;
}

cg::Shader::Shader() : id(0) {}

std::string getShaderSource(GLuint shader) {
    constexpr size_t buf_size = 1u << 20u;
    static char buf[buf_size];
    glGetShaderSource(shader, buf_size, nullptr, buf);
    return buf;
}

cg::Shader::Shader(const char *vertexShaderSource, const char *fragmentShaderSource) {
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
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
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
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        fprintf(stderr, "ERROR::SHADER::PROGRAM::LINKING_FAILED\n%s\n", infoLog);
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    id = shaderProgram;
}

cg::Shader::Shader(cg::Shader &&b) noexcept: id(b.id) {
    b.id = 0;
}

void cg::Shader::use() const {
    glUseProgram(id);
}

cg::Shader::~Shader() {
    if (id) {
        glDeleteProgram(id);
    }
}

cg::Shader &cg::Shader::operator=(cg::Shader &&shader) noexcept {
    if (id) {
        glDeleteProgram(id);
    }
    id = shader.id;
    shader.id = 0;
    return *this;
}

const char *shaderPassVertexShader = R"(
#version 330 core

layout (location = 0) in vec2 inTexCoord;
out vec2 texCoord;

uniform sampler2D screenTexture;

void main() {
    texCoord = inTexCoord;
    gl_Position = vec4(inTexCoord * 2.0 - 1.0, 0.0, 1.0);
}
)";

namespace cg {

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

        shader.use();
        glUniform1i(glGetUniformLocation(shader.id, "screenTexture"), 0);
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
        colorBuffer.generate(width, height, GL_RGB);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorBuffer.tex(), 0);
        // create a renderbuffer object for depth and stencil attachment (we won't be sampling these)
        glGenRenderbuffers(1, &rbo);
        glBindRenderbuffer(GL_RENDERBUFFER, rbo);
        // use a single renderbuffer object for both a depth AND stencil buffer.
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);
        // now that we actually created the framebuffer and added all attachments we want to check if it is actually complete now
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            fputs("ERROR::FRAMEBUFFER:: Framebuffer is not complete!", stderr);
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
    for (auto &pass : _passes) {
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
