#include <texture.h>

#include <map>
#include <optional>

static float TEX_WHITE[] = {1.0f, 1.0f, 1.0f, 1.0f};
static float TEX_BLACK[] = {0.0f, 0.0f, 0.0f, 1.0f};
static float TEX_NORMAL[] = {0.0f, 0.0f, 1.0f, 1.0f};
static float TEX_TRANSPARENT[] = {0.0f, 0.0f, 0.0f, 0.0f};

namespace cg {
struct TextureImpl {
    GLuint tex;

    explicit TextureImpl(GLuint tex) noexcept: tex(tex) {}

    TextureImpl() noexcept: tex(0u) {}

    TextureImpl(const TextureImpl &) = delete;

    TextureImpl(TextureImpl &&rhs) noexcept: tex(rhs.tex) {
        rhs.tex = 0;
    }

    void generate(int width, int height, GLenum format) {
        if (tex) {
            glDeleteTextures(1, &tex);
            tex = 0u;
        }
        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        tex = texture;
    }

    ~TextureImpl() {
        if (tex) {
            glDeleteTextures(1, &tex);
        }
    }
};
}

cg::Texture cg::Texture::defaultTexture(cg::DefaultTexture type) {
    static std::map<cg::DefaultTexture, cg::Texture> defaultTexMap;
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
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGBA, GL_FLOAT, data);
//    glGenerateMipmap(GL_TEXTURE_2D);

    return defaultTexMap.emplace(type, tex).first->second;
}

cg::Texture::Texture(GLuint tex) : impl(std::make_shared<TextureImpl>(tex)) {}

cg::Texture::Texture() : impl(std::make_shared<TextureImpl>()) {}

GLuint cg::Texture::tex() const {
    return impl->tex;
}

void cg::Texture::generate(int width, int height, GLenum format) {
    impl->generate(width, height, format);
}
