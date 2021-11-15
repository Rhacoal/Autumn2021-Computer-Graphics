#include <texture.h>

#include <map>
#include <optional>
#include <stb_image.h>

static float TEX_WHITE[] = {1.0f, 1.0f, 1.0f, 1.0f};
static float TEX_BLACK[] = {0.0f, 0.0f, 0.0f, 1.0f};
static float TEX_NORMAL[] = {0.0f, 0.0f, 1.0f, 1.0f};
static float TEX_TRANSPARENT[] = {0.0f, 0.0f, 0.0f, 0.0f};

namespace cg {
struct TextureImpl {
    GLuint tex;
    std::vector<float> _data;
    GLint _width = 0, _height = 0;

    explicit TextureImpl(GLuint tex) noexcept: tex(tex) {}

    TextureImpl() noexcept: tex(0u) {}

    TextureImpl(const TextureImpl &) = delete;

    TextureImpl(TextureImpl &&rhs) noexcept: tex(rhs.tex) {
        rhs.tex = 0;
    }

    float *rawData() {
        return _data.data();
    }

    const float *rawData() const {
        return _data.data();
    }

    auto begin() const {
        return _data.begin();
    }

    auto end() const {
        return _data.end();
    }

    GLint width() const {
        return _width;
    }

    GLint height() const {
        return _height;
    }

    void generate(int width, int height, GLint format) {
        if (tex) {
            glDeleteTextures(1, &tex);
            tex = 0u;
        }
        _width = width;
        _height = height;
        GLuint texture;
        glGenTextures(1, &texture);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        _data.clear();
        _data.resize(width * height * 4, 0.0f);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        tex = texture;
    }

    constexpr static uint8_t NO_COMPONENT = 255;

    template<typename T, uint32_t scale, uint8_t components = 4, uint8_t R = 0, uint8_t G = 1, uint8_t B = 2, uint8_t A = 3>
    void copyData(const void *raw) {
        const T *data = reinterpret_cast<const T *>(raw);
        for (size_t i = 0; i < _width * _height; ++i) {
            if constexpr (R >= 0) {
                _data[i * 4 + 0] = static_cast<float>(data[i * components + R]) / scale;
            }
            if constexpr (G >= 0) {
                _data[i * 4 + 1] = static_cast<float>(data[i * components + G]) / scale;
            }
            if constexpr (B >= 0) {
                _data[i * 4 + 2] = static_cast<float>(data[i * components + B]) / scale;
            }
            if constexpr (A >= 0) {
                _data[i * 4 + 3] = static_cast<float>(data[i * components + A]) / scale;
            }
        }
    }

    template<uint8_t components = 4, uint8_t R = 0, uint8_t G = 1, uint8_t B = 2, uint8_t A = 3>
    void copyDataType(GLenum type, const void *data) {
        switch (type) {
            case GL_FLOAT:
                copyData<float, 1, components, R, G, B, A>(data);
                break;
            case GL_UNSIGNED_BYTE:
                copyData<uint8_t, 255, components, R, G, B, A>(data);
                break;
            default:
                throw std::runtime_error("unexpected data type" + std::to_string(type));
        }
    }

    void copyDataFormatType(GLenum format, GLenum type, const void *data) {
        switch (format) {
            case GL_RGB:
                copyDataType<3, 0, 1, 2, NO_COMPONENT>(type, data);
                break;
            case GL_RGBA:
                copyDataType<4, 0, 1, 2, 3>(type, data);
                break;
            case GL_RED:
                copyDataType<1, 0, NO_COMPONENT, NO_COMPONENT, NO_COMPONENT>(type, data);
                break;
            case GL_RG:
                copyDataType<2, 0, 1, NO_COMPONENT, NO_COMPONENT>(type, data);
                break;
            case GL_BGR:
                copyDataType<3, 2, 1, 0, NO_COMPONENT>(type, data);
                break;
            case GL_BGRA:
                copyDataType<4, 2, 1, 0, 3>(type, data);
                break;
            default:
                throw std::runtime_error("unexpected data format" + std::to_string(format));
        }
    }

    void setData(int width, int height, GLint internalFormat, GLenum format, GLenum type, const void *data) {
        GLuint texture = tex;
        if (!texture) {
            glGenTextures(1, &texture);
        }
        _width = width;
        _height = height;
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, type, data);
        _data.resize(width * height * 4);
        if (format == GL_RGBA && type == GL_FLOAT) {
            memcpy(_data.data(), data, _data.size() * sizeof(float));
        } else {
            copyDataFormatType(format, type, data);
        }
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glGenerateMipmap(texture);
        tex = texture;
    }

    ~TextureImpl() {
        if (tex) {
            glDeleteTextures(1, &tex);
        }
    }
};
}

cg::CubeTexture::CubeTexture(GLuint tex) : impl(std::make_shared<TextureImpl>(tex)) {}

cg::CubeTexture::CubeTexture() : impl(std::make_shared<TextureImpl>()) {}

GLuint cg::CubeTexture::tex() const {
    return impl ? impl->tex : 0;
}

cg::CubeTexture cg::CubeTexture::load(const char **faces) {
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < 6; i++) {
        if (!faces[i]) continue;
        unsigned char *data = stbi_load(faces[i], &width, &height, &nrChannels, 3);
        if (data) {
            // by default, assumes that skybox is in srgb
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                0, GL_SRGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
            );
            stbi_image_free(data);
        } else {
            fprintf(stderr, "failed to load: %s\n", faces[i]);
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return CubeTexture(textureID);
}

cg::Texture cg::Texture::defaultTexture(cg::Texture::DefaultTexture type) {
    static std::map<cg::Texture::DefaultTexture, cg::Texture> defaultTexMap;
    auto it = defaultTexMap.find(type);
    if (it != defaultTexMap.end()) {
        return it->second;
    }

    float *data = nullptr;
    switch (type) {
        case DefaultTexture::WHITE:
            data = TEX_WHITE;
            break;
        case DefaultTexture::BLACK:
            data = TEX_BLACK;
            break;
        case DefaultTexture::DEFAULT_NORMAL:
            data = TEX_NORMAL;
            break;
        case DefaultTexture::TRANSPARENT:
            data = TEX_TRANSPARENT;
            break;
    }

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    // set the texture wrapping/filtering options (on the currently bound texture object)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // load and generate the texture
    // no gamma correction needed
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGBA, GL_FLOAT, data);
    // mipmap is not needed
//    glGenerateMipmap(GL_TEXTURE_2D);

    auto texture = defaultTexMap.emplace(type, tex).first->second;
    texture.setData(1, 1, GL_RGB, GL_RGBA, GL_FLOAT, data);
    return texture;
}

cg::Texture::Texture(GLuint tex) : impl(std::make_shared<TextureImpl>(tex)) {}

cg::Texture::Texture() : impl(std::make_shared<TextureImpl>()) {}

GLuint cg::Texture::tex() const {
    return impl ? impl->tex : 0;
}

int cg::Texture::width() const {
    return impl->width();
}

int cg::Texture::height() const {
    return impl->height();
}

void cg::Texture::generate(int width, int height, GLint internalFormat) {
    impl->generate(width, height, internalFormat);
}

void cg::Texture::setData(int width, int height, GLint internalFormat, GLenum format, GLenum type, const void *data) {
    impl->setData(width, height, internalFormat, format, type, data);
}

const float *cg::Texture::data() const {
    return impl->rawData();
}

std::vector<float>::const_iterator cg::Texture::begin() const {
    return impl->begin();
}

std::vector<float>::const_iterator cg::Texture::end() const {
    return impl->end();
}
