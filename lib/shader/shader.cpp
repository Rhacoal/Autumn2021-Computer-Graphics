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
    GLint length = 0;
    glShaderSource(vertexShader, 1, &vertexShaderSource, &length);
    glCompileShader(vertexShader);

    auto vs = getShaderSource(vertexShader);
    printf("error? %d %d %s\n", glGetError(), length, vertexShaderSource);

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
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, &length);
    glCompileShader(fragmentShader);
    // check for compilation errors
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        fprintf(stderr, "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n%s\n", infoLog);
    }

    auto fs = getShaderSource(fragmentShader);

    // link program
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    puts(vs.c_str());
    puts(fs.c_str());
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
