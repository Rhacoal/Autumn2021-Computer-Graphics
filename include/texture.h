#ifndef ASSIGNMENT_TEXTURE_H
#define ASSIGNMENT_TEXTURE_H

#include <cg_common.h>
#include <cg_fwd.h>

#include <utility>
#include <memory>

namespace cg {
struct TextureImpl;

class Texture {
private:
    std::shared_ptr<TextureImpl> impl;
public:
    enum class Encoding {
        LINEAR = GL_RGB, SRGB = GL_SRGB,
    };

    enum class DefaultTexture {
        WHITE, BLACK, DEFAULT_NORMAL, TRANSPARENT,
    };

    explicit Texture(GLuint tex);

    Texture();

    Texture(const Texture &) = default;

    Texture(Texture &&) = default;

    Texture &operator=(const Texture &rhs) = default;

    void generate(int width, int height, GLint internalFormat);

    void setData(int width, int height, GLint internalFormat, GLenum format, GLenum type, const void *data);

    static Texture defaultTexture(DefaultTexture defaultTexture);

    GLuint tex() const;
};


class CubeTexture {
private:
    std::shared_ptr<TextureImpl> impl;
public:
    explicit CubeTexture(GLuint tex);

    CubeTexture();

    CubeTexture(const CubeTexture &) = default;

    CubeTexture(CubeTexture &&) = default;

    CubeTexture &operator=(const CubeTexture &) = default;

    /**
     * Loads cube texture.
     * @param faces paths to 6 faces
     * @return loaded cube texture
     */
    static CubeTexture load(const char **faces);

    GLuint tex() const;
};
}

#endif //ASSIGNMENT_TEXTURE_H
