#ifndef ASSIGNMENT_MATERIAL_H
#define ASSIGNMENT_MATERIAL_H

#include <cg_common.h>
#include <cg_fwd.h>
#include <shader.h>

#include <string>
#include <optional>

namespace cg {

struct _textureImpl {
    std::optional<GLuint> tex;

    explicit _textureImpl(GLuint tex) noexcept: tex(tex) {}

    _textureImpl(const _textureImpl &) = delete;

    _textureImpl(_textureImpl &&rhs) noexcept: tex(rhs.tex) {
        rhs.tex.reset();
    }

    ~_textureImpl() {
        if (tex.has_value()) {
            glDeleteTextures(1, &tex.value());
        }
    }
};

enum class DefaultTexture {
    WHITE, BLACK, DEFAULT_NORMAL, TRANSPARENT,
};

class Texture {
private:
    std::shared_ptr<_textureImpl> impl;
public:
    explicit Texture(GLuint tex) : impl(std::make_shared<_textureImpl>(tex)) {}

    Texture(const Texture &) = default;

    Texture(Texture &&) = default;

    static Texture defaultTexture(DefaultTexture defaultTexture);

    GLuint tex() const {
        return impl->tex.value();
    }
};

/**
 * Material of an object, including color, textures, shaders and so on.
 */
class Material {
    static int latest_id;
public:
    const int id;

    Material() : id(latest_id++) {}

    Material(const Material &) = delete;

    Material(Material &&) = delete;

    virtual GLuint shaderProgram(Scene &sc, Camera &cam, ProgramArguments &pargs) = 0;

    virtual bool isTransparent() const noexcept = 0;
};

template<typename T>
class WeakHolder {
    static std::weak_ptr<T> weak;
public:
    static std::shared_ptr<T> getInstance() {
        auto ret = weak.lock();
        if (ret) {
            return ret;
        }
        ret = std::make_shared<T>();
        weak = ret;
        return weak.lock();
    }
};

class PhongMaterial : public Material {
    typedef std::tuple<size_t, size_t> cache_key_t;
    cache_key_t prevKey;
public:
    Shader shader;
    std::optional<Texture> diffuse = Texture::defaultTexture(DefaultTexture::WHITE);
    glm::vec4 color;
    glm::float32 shininess;

    PhongMaterial() : color(1.0f), shininess(25.0f) {}

    GLuint shaderProgram(Scene &sc, Camera &cam, ProgramArguments &pargs) noexcept override;

    bool isTransparent() const noexcept override {
        return false;
    }
};

struct StandardMaterial : public Material {
    std::optional<Texture> albedo;
    std::optional<Texture> mra;
    std::optional<Texture> normal;
};
}

#endif //ASSIGNMENT_MATERIAL_H
