#include <shader.h>

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
