#ifndef ASSIGNMENT_GEOMETRY_H
#define ASSIGNMENT_GEOMETRY_H

#include <cg_common.h>

#include <optional>
#include <vector>
#include <string>
#include <map>

namespace cg {
class Geometry {
public:
    static constexpr GLenum gl_draw_type = GL_TRIANGLES;
    typedef std::vector<float> buffer_t;
    typedef struct {
        int layout_index;
        buffer_t buf;
        unsigned int itemSize;
        bool normalized;
    } attrib;

    Geometry() = default;

    ~Geometry() {
        if (inited) {
            glDeleteVertexArrays(1, &vao);
            glDeleteBuffers(1, &vbo);
            glDeleteBuffers(1, &ebo);
        }
    }

    Geometry(const Geometry &) = delete;

    Geometry(Geometry &&) noexcept;

    bool hasAttribute(const std::string &name) const {
        return attribs.find(name) != attribs.end();
    }

    bool removeAttribute(const std::string &name) {
        auto it = attribs.find(name);
        if (it == attribs.end()) return false;
        attribs.erase(it);
        _invalidate_vbo();
        return true;
    }

    template<size_t N>
    void addAttribute(const std::string &name, const float (&array)[N], unsigned int itemSize) {
        addAttribute(name, array, N, itemSize);
    }

    void addAttribute(const std::string &name, const float *array, size_t array_size, unsigned int itemSize) {
        attribs.emplace(name, attrib{-1, buffer_t(array, array + array_size), itemSize, false});
        _invalidate_vbo();
    }

    template<size_t N>
    void addIndices(const unsigned int (&array)[N]) {
        addIndices(array, N);
    }

    void addIndices(const unsigned int *array, size_t arrayLength) {
        if (!array) {
            indices.reset();
            _invalidate_ebo();
            return;
        }
        if (!indices.has_value()) {
            indices.emplace(arrayLength);
        } else {
            indices->resize(arrayLength);
        }
        std::copy_n(array, arrayLength, indices->begin());
        _invalidate_ebo();
    }

    bool hasIndices() const noexcept {
        return indices.has_value();
    }

    void mergeGeometry(const Geometry &geo);

    void bindVAO(GLuint shaderProgram);

    GLsizei elementCount() const {
        if (indices.has_value()) {
            return static_cast<GLsizei>(indices->size());
        }
        return count;
    }

private:
    void _invalidate_vbo() {
        _need_update = true;
        _need_re_layout = true;
    }

    void _invalidate_ebo() {
        _need_update_indices = true;
    }

    GLuint vao{}, vbo{}, ebo{};
    std::map<std::string, attrib> attribs;
    std::vector<GLuint> _used_positions;
    std::optional<std::vector<unsigned int>> indices;
    GLuint prev_shaderProgram = 0;
    bool _need_update = true;
    bool _need_re_layout = true;
    bool _need_update_indices = true;
    bool inited = false;
    GLsizei count = 0;
};

class BoxGeometry : public Geometry {
public:
    BoxGeometry(float sizeX, float sizeY, float sizeZ);
};
}

#endif //ASSIGNMENT_GEOMETRY_H