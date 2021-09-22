#ifndef ASSIGNMENT_COMMON_H
#define ASSIGNMENT_COMMON_H

#include <GL/glew.h>

namespace cgpa {
struct vertex_array_object {
    GLuint vertexBuffer{};

    vertex_array_object() {
        // This will identify our vertex buffer
        glGenBuffers(1, &vertexBuffer);
    }

    vertex_array_object(const vertex_array_object &) = delete;

    vertex_array_object(vertex_array_object &&other) : vertexBuffer(other.vertexBuffer) {
        other.vertexBuffer = -1;
    }

    ~vertex_array_object() {
        if (vertexBuffer != -1) {
            glDeleteBuffers(1, &vertexBuffer);
        }
    }
};
}

#endif //ASSIGNMENT_COMMON_H
