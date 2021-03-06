#include <geometry.h>

cg::BaseGeometry::BaseGeometry(cg::BaseGeometry &&b) noexcept: vao(b.vao), vbo(b.vbo), ebo(b.ebo),
                                                               attribs(std::move(b.attribs)) {
    b.vao = b.vbo = b.ebo = 0;
    b.inited = false;
}

void cg::BaseGeometry::bindVAO(GLuint shaderProgram) {
    if (!inited) {
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);
        inited = true;
        _invalidate_vbo();
    }
    if (shaderProgram != prev_shaderProgram) {
        _need_re_layout = true;
        prev_shaderProgram = shaderProgram;
    }
    glBindVertexArray(vao);
    if (_need_update) {
        int buffer_size = 0;
        ssize_t vertexCount = -1;
        unsigned int attrib_size = 0;
        for (const auto &[name, attrib]: attribs) {
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
        for (const auto &[name, attrib]: attribs) {
            for (ssize_t i = 0; i < vertexCount; ++i) {
                std::copy_n(attrib.buf.begin() + attrib.itemSize * i,
                    attrib.itemSize,
                    buf.begin() + attrib_size * i + offset);
            }
            offset += attrib.itemSize;
        }
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, buffer_size * sizeof(float), buf.data(), GL_STATIC_DRAW);
        _need_update = false;
    }
    if (_need_re_layout) {
        for (auto pos: _used_positions) {
            glDisableVertexAttribArray(pos);
        }
        _used_positions.clear();
        unsigned int offset = 0;
        unsigned int stride = 0;
        for (const auto &[name, attrib]: attribs) {
            stride += attrib.itemSize * sizeof(float);
        }
        for (auto &[name, attrib]: attribs) {
            GLint location = glGetAttribLocation(shaderProgram, name.c_str());
            attrib.layoutIndex = location;
            if (location == -1) {
                // attribute not found
            } else {
                glVertexAttribPointer(location, attrib.itemSize, GL_FLOAT, GL_FALSE, stride,
                    (void *) (offset * sizeof(float)));
                glEnableVertexAttribArray(location);
                _used_positions.push_back(location);
            }
            offset += attrib.itemSize;
        }
        _need_re_layout = false;
    }
    if (_need_update_indices) {
        if (hasIndices()) {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * indices->size(), indices->data(),
                GL_STATIC_DRAW);
        }
        _need_update_indices = false;
    }
}

void cg::BaseGeometry::mergeGeometry(const cg::BaseGeometry &geo) {
    // TODO support this
}

template<int u, int v, int w, int sign>
void createPlane(float *&positions, float *&normals, float *&texcoords, unsigned int *&indices, int &index0,
                 float (&sizes)[3],
                 bool front, bool back) {
    float half_u = sizes[u] / 2, half_v = sizes[v] / 2, half_w = sign * (sizes[w] / 2);
    float points[] = {
        +half_u, +half_v, half_w,
        -sign * half_u, +sign * half_v, half_w,
        -half_u, -half_v, half_w,
        +sign * half_u, -sign * half_v, half_w,
    };
    float uvs[] = {
        1.0f, 1.0f,
        -sign * 0.5f + 0.5f, +sign * 0.5f + 0.5f,
        0.0f, 0.0f,
        +sign * 0.5f + 0.5f, -sign * 0.5f + 0.5f,
    };
    // +u,+v -> -u,+v -> -u,-v -> +u,-v
    for (int i = 0; i < 4; ++i) {
        positions[u] = points[i * 3 + 0];
        positions[v] = points[i * 3 + 1];
        positions[w] = points[i * 3 + 2];
        texcoords[0] = uvs[i * 2 + 0];
        texcoords[1] = uvs[i * 2 + 1];
        positions += 3;
        texcoords += 2;
    }
    for (int i = 0; i < 4; ++i) {
        normals[u] = 0.0f;
        normals[v] = 0.0f;
        normals[w] = sign;
        normals += 3;
    }
    constexpr int face_indices[] = {
        0, 1, 2, 0, 2, 3,
        0, 2, 1, 0, 3, 2,
    };
    if (front) {
        for (int i = 0; i < 6; ++i) {
            *(indices++) = index0 + face_indices[i];
        }
    }
    if (back) {
        for (int i = 6; i < 12; ++i) {
            *(indices++) = index0 + face_indices[i];
        }
    }
    index0 += 4;
}

cg::BoxGeometry::BoxGeometry(float sizeX, float sizeY, float sizeZ, Side side) {
    float positions[12 * 6];
    float normals[12 * 6];
    float texcoords[8 * 6];
    unsigned int indices[6 * 6 * 2];
    constexpr int x = 0, y = 1, z = 2;
    int index0 = 0;
    float sizes[] = {sizeX, sizeY, sizeZ};
    const auto funcs = {
        createPlane<y, z, x, +1>, // px
        createPlane<y, z, x, -1>, // nx
        createPlane<x, y, z, +1>, // pz
        createPlane<x, y, z, -1>, // nz
        createPlane<z, x, y, +1>, // py
        createPlane<z, x, y, -1>, // ny
    };
    float *ptr_pos = positions, *ptr_norm = normals, *ptr_uv = texcoords;
    unsigned int *ptr_idx = indices;
    for (auto func: funcs) {
        func(ptr_pos, ptr_norm, ptr_uv, ptr_idx, index0, sizes, side != Side::BackSide, side != Side::FrontSide);
    }
    addAttribute("position", positions, 3);
    addAttribute("normal", normals, 3);
    addAttribute("texcoord", texcoords, 2);
    if (side == Side::DoubleSide) {
        addIndices(indices, 72);
    } else {
        addIndices(indices, 36);
    }
}

cg::SphereGeometry::SphereGeometry(float radius, int widthSegments, int heightSegments) {
    int vertexCount = (widthSegments + 1) * (heightSegments + 1);
    std::vector<float> position(vertexCount * 3);
    std::vector<float> normal(vertexCount * 3);
    std::vector<float> texcoord(vertexCount * 2);
    std::vector<unsigned int> indices(widthSegments * (2 * heightSegments - 1) * 3);
    int index = 0;
    for (int i = 0; i <= heightSegments; ++i) {
        float v = (float) i / (float) heightSegments;
        float theta = math::pi<float>() * v;
        float y = cos(theta);
        for (int j = 0; j <= widthSegments; ++j) {
            float u = (float) j / (float) widthSegments;
            float phi = math::pi<float>() * 2 * u;
            float x = cos(phi) * sin(theta);
            float z = -sin(phi) * sin(theta);
            position[index * 3 + 0] = radius * x;
            position[index * 3 + 1] = radius * y;
            position[index * 3 + 2] = radius * z;
            normal[index * 3 + 0] = x;
            normal[index * 3 + 1] = y;
            normal[index * 3 + 2] = z;
            texcoord[index * 2 + 0] = u;
            texcoord[index * 2 + 1] = v;
            ++index;
        }
    }
    const auto getIndex = [&](int i, int j) {
        return i * (widthSegments + 1) + j;
    };
    index = 0;
    for (int i = 0; i < heightSegments; ++i) {
        for (int j = 0; j < widthSegments; ++j) {
            int v0 = getIndex(i, j);
            int v1 = getIndex(i + 1, j);
            int v2 = getIndex(i + 1, j + 1);
            int v3 = getIndex(i, j + 1);
            if (i != heightSegments) {
                indices[index++] = v0;
                indices[index++] = v1;
                indices[index++] = v2;
            }
            if (i != 0) {
                indices[index++] = v0;
                indices[index++] = v2;
                indices[index++] = v3;
            }
        }
    }
    addAttribute("position", position, 3);
    addAttribute("normal", normal, 3);
    addAttribute("texcoord", texcoord, 2);
    addIndices(indices);
}

cg::PlaneGeometry::PlaneGeometry(float sizeX, float sizeZ, Side side) {
    float positions[12];
    float normals[12];
    float texcoord[8];
    unsigned int indices[6 * 2];
    constexpr int x = 0, y = 1, z = 2;
    int index0 = 0;
    float sizes[] = {sizeX, 0, sizeZ};
    float *ptr_pos = positions, *ptr_norm = normals, *ptr_texcoord = texcoord;
    unsigned int *ptr_idx = indices;
    createPlane<z, x, y, +1>(ptr_pos, ptr_norm, ptr_texcoord, ptr_idx, index0, sizes,
        side != Side::BackSide, side != Side::FrontSide);
    addAttribute("position", positions, 3);
    addAttribute("normal", normals, 3);
    addAttribute("texcoord", texcoord, 2);
    if (side == Side::DoubleSide) {
        addIndices(indices, 12);
    } else {
        addIndices(indices, 6);
    }
}
