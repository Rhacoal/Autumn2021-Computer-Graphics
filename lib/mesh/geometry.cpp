#include <geometry.h>

cg::Geometry::Geometry(cg::Geometry &&b) noexcept: vao(b.vao), vbo(b.vbo), ebo(b.ebo),
                                                   attribs(std::move(b.attribs)) {
    b.vao = b.vbo = b.ebo = 0;
    b.inited = false;
}

void cg::Geometry::bindVAO(GLuint shaderProgram) {
    if (!inited) {
        check_err(glGenVertexArrays(1, &vao));
        check_err(glGenBuffers(1, &vbo));
        check_err(glGenBuffers(1, &ebo));
        inited = true;
        _invalidate_vbo();
    }
    if (shaderProgram != prev_shaderProgram) {
        _need_re_layout = true;
        prev_shaderProgram = shaderProgram;
    }
    check_err(glBindVertexArray(vao));
    if (_need_update) {
        int buffer_size = 0;
        ssize_t vertexCount = -1;
        unsigned int attrib_size = 0;
        for (const auto &[name, attrib] : attribs) {
            buffer_size += attrib.buf.size();
            attrib_size += attrib.itemSize;
            if (vertexCount != -1 && vertexCount * attrib.itemSize != attrib.buf.size()) {
                // inconsistent vertex count!
                fprintf(stderr, "invalid vertex count for attribute %s\n", name.c_str());
                return;
            } else {
                vertexCount = attrib.buf.size() / attrib.itemSize;
            }
            count = vertexCount;
        }
        buffer_t buf(buffer_size);
        unsigned int offset = 0;
        for (const auto &[name, attrib] : attribs) {
            for (ssize_t i = 0; i < vertexCount; ++i) {
                std::copy_n(attrib.buf.begin() + attrib.itemSize * i,
                            attrib.itemSize,
                            buf.begin() + attrib_size * i + offset);
            }
            offset += attrib.itemSize;
        }
        check_err(glBindBuffer(GL_ARRAY_BUFFER, vbo));
        check_err(glBufferData(GL_ARRAY_BUFFER, buffer_size * sizeof(float), buf.data(), GL_STATIC_DRAW));
        _need_update = false;
    }
    if (_need_re_layout) {
        for (auto pos : _used_positions) {
            glDisableVertexAttribArray(pos);
        }
        _used_positions.clear();
        unsigned int offset = 0;
        unsigned int stride = 0;
        for (const auto &[name, attrib] : attribs) {
            stride += attrib.itemSize * sizeof(float);
        }
        for (const auto &[name, attrib] : attribs) {
            GLint location = glGetAttribLocation(shaderProgram, name.c_str());
            if (location == -1) {
                // attribute not found
            } else {
                check_err(glVertexAttribPointer(location, attrib.itemSize, GL_FLOAT, GL_FALSE, stride,
                                                (void *) (offset * sizeof(float))));
                check_err(glEnableVertexAttribArray(location));
                _used_positions.push_back(location);
            }
            offset += attrib.itemSize;
        }
        _need_re_layout = false;
    }
    if (_need_update_indices) {
        if (hasIndices()) {
            check_err(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo));
            check_err(glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * indices->size(), indices->data(),
                                   GL_STATIC_DRAW));
        }
        _need_update_indices = false;
    }
}

void cg::Geometry::mergeGeometry(const cg::Geometry &geo) {
    // TODO support this
}

cg::BoxGeometry::BoxGeometry(float sizeX, float sizeY, float sizeZ) {
    float halfX = sizeX / 2, halfY = sizeY / 2, halfZ = sizeZ / 2;
    float positions[] = {
        -halfX, -halfY, -halfZ,
        halfX, -halfY, -halfZ,
        halfX, halfY, -halfZ,
        -halfX, halfY, -halfZ,
        -halfX, -halfY, halfZ,
        halfX, -halfY, halfZ,
        halfX, halfY, halfZ,
        -halfX, halfY, halfZ,
    };
    float colors[] = {
        0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 1.0f,
        1.0f, 0.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        0.0f, 1.0f, 1.0f,
    };
    unsigned int indices[] = {
        0, 1, 2,
        0, 2, 3,
        1, 5, 6,
        1, 6, 2,
        5, 4, 7,
        5, 7, 6,
        4, 0, 3,
        4, 3, 7,
        2, 6, 7,
        2, 7, 3,
        0, 4, 5,
        0, 5, 1,
    };
    addAttribute("position", positions, 3);
    addAttribute("color", colors, 3);
    addIndices(indices);
}
