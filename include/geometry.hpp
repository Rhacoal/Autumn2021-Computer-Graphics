#ifndef ASSIGNMENT_GEOMETRY_HPP
#define ASSIGNMENT_GEOMETRY_HPP

#include <cgpa_common.hpp>
#include <GL/glew.h>

namespace cgpa {
template<typename T>
struct geometry_traits {
    typedef T geometry_type;
    static constexpr GLenum gl_draw_type = T::gl_draw_type;

    void bind_attributes(const T &geometry) {
        geometry.bind_attributes();
    }
};

class triangle_geometry {
public:
    static constexpr GLenum gl_draw_type = GL_TRIANGLES;
    vertex_array_object vertices;

    triangle_geometry() = default;

    triangle_geometry(const triangle_geometry &) = delete;

    triangle_geometry(triangle_geometry &&) = delete;

    void bind_attributes() {
    }
};
}

#endif //ASSIGNMENT_GEOMETRY_HPP
