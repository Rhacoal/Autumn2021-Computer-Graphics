#ifndef ASSIGNMENT_GEOMETRY_H
#define ASSIGNMENT_GEOMETRY_H

#include <cg_common.h>

#include <optional>
#include <vector>
#include <string>
#include <map>

namespace cg {
/**
 * BaseGeometry is a wrapper for vertex array objects.
 *
 * For specific primitives, see:
 * class Geometry for triangle primitives
 */
class BaseGeometry {
public:
    typedef std::vector<float> buffer_t;
    typedef struct {
        int layoutIndex;
        buffer_t buf;
        unsigned int itemSize;
        bool normalized;
    } Attribute;

    BaseGeometry() = default;

    BaseGeometry(const BaseGeometry &) = delete;

    ~BaseGeometry() {
        if (inited) {
            glDeleteVertexArrays(1, &vao);
            glDeleteBuffers(1, &vbo);
            glDeleteBuffers(1, &ebo);
        }
    }

    BaseGeometry(BaseGeometry &&) noexcept;

    virtual GLenum glPrimitiveType() const noexcept = 0;

    bool hasAttribute(const std::string &name) const {
        return attribs.find(name) != attribs.end();
    }

    std::optional<const Attribute *> getAttribute(const std::string &name) const {
        auto it = attribs.find(name);
        if (it == attribs.end()) {
            return {};
        }
        return std::make_optional(&it->second);
    }

    bool removeAttribute(const std::string &name) {
        auto it = attribs.find(name);
        if (it == attribs.end()) return false;
        attribs.erase(it);
        _invalidate_vbo();
        return true;
    }

    template<typename ContainerType>
    void addAttribute(const std::string &name, ContainerType &&container, unsigned int itemSize) {
        addAttribute(name, std::data(container), std::size(container), itemSize);
    }

    void addAttribute(const std::string &name, const float *array, size_t array_size, unsigned int itemSize) {
        attribs.emplace(name, Attribute{-1, buffer_t(array, array + array_size), itemSize, false});
        _invalidate_vbo();
    }

    template<typename ContainerType>
    void addIndices(const std::string &name, ContainerType &&container) {
        addIndices(name, std::data(container), std::size(container));
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

    const std::optional<std::vector<unsigned int>> &getIndices() const noexcept {
        return indices;
    }

    void mergeGeometry(const BaseGeometry &geo);

    virtual void bindVAO(GLuint shaderProgram);

    GLsizei elementCount() const {
        if (indices.has_value()) {
            return static_cast<GLsizei>(indices->size());
        }
        return count;
    }

protected:
    void _invalidate_vbo() {
        _need_update = true;
        _need_re_layout = true;
    }

    void _invalidate_ebo() {
        _need_update_indices = true;
    }

    GLuint vao{}, vbo{}, ebo{};
    std::map<std::string, Attribute> attribs;
    std::vector<GLuint> _used_positions;
    std::optional<std::vector<unsigned int>> indices;
    GLuint prev_shaderProgram = 0;
    bool _need_update = true;
    bool _need_re_layout = true;
    bool _need_update_indices = true;
    bool inited = false;
    GLsizei count = 0;
};

class MeshGeometry : public BaseGeometry {
public:
    GLenum glPrimitiveType() const noexcept override {
        return GL_TRIANGLES;
    }

    MeshGeometry() = default;

    template<typename F>
    void traverseVertices(F &&func) {
        auto it = attribs.find("position");
        if (it == attribs.end()) return;
        const auto &pos = it->second.buf;
        if (indices.has_value()) {
            for (unsigned int pi : *indices) {
                func(pos[pi * 3], pos[pi * 3 + 1], pos[pi * 3 + 2]);
            }
        } else {
            for (unsigned int i = 0; i + 2 < pos.size(); i += 3) {
                func(pos[i], pos[i + 1], pos[i + 2]);
            }
        }
    }
};

enum class Side {
    FrontSide, BackSide, DoubleSide
};

class BoxGeometry : public MeshGeometry {
public:
    BoxGeometry(float sizeX, float sizeY, float sizeZ, Side side = Side::FrontSide);
};

class SphereGeometry : public MeshGeometry {
public:
    SphereGeometry(float radius, int widthSegments = 20, int heightSegments = 20);
};
}

#endif //ASSIGNMENT_GEOMETRY_H
