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

cg::Texture::Texture(cg::DefaultTexture type) : impl(defaultTexture(type).impl) {
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
    // no gamma correction needed
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGBA, GL_FLOAT, data);
    // mipmap is not needed
//    glGenerateMipmap(GL_TEXTURE_2D);

    return defaultTexMap.emplace(type, tex).first->second;
}

cg::Texture::Texture(GLuint tex) : impl(std::make_shared<TextureImpl>(tex)) {}

cg::Texture::Texture() : impl(std::make_shared<TextureImpl>()) {}

GLuint cg::Texture::tex() const {
    return impl ? impl->tex : 0;
}

void cg::Texture::generate(int width, int height, GLenum format) {
    impl->generate(width, height, format);
}
