#ifndef ASSIGNMENT_TEXTURE_H
#define ASSIGNMENT_TEXTURE_H

#include <cg_common.h>
#include <cg_fwd.h>

#include <utility>
#include <memory>

namespace cg {
struct TextureImpl;

enum class DefaultTexture {
    WHITE, BLACK, DEFAULT_NORMAL, TRANSPARENT,
};

class Texture {
private:
    std::shared_ptr<TextureImpl> impl;
public:
    explicit Texture(GLuint tex);

    Texture();

    Texture(const Texture &) = default;

    Texture(Texture &&) = default;

    void generate(int width, int height, GLenum format);

    static Texture defaultTexture(DefaultTexture defaultTexture);

    GLuint tex() const;
};
}

#endif //ASSIGNMENT_TEXTURE_H
