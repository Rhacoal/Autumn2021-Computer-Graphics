#ifndef ASSIGNMENT_MATERIAL_H
#define ASSIGNMENT_MATERIAL_H

#include <cg_common.h>
#include <cg_fwd.h>
#include <shader.h>

#include <string>
#include <optional>
#include <memory>

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

    /**
     * Called before switching to this shader program. This method is called only once per frame for all objects sharing
     * this same material.
     *
     * @return shader program used
     */
    virtual GLuint useShaderProgram(Scene &scene, Camera &camera, ProgramArguments &pargs) = 0;

    /**
     * Called on each object using this material to update uniforms for this particular object.
     */
    virtual void updateUniforms(Object3D *object, Camera &camera) = 0;

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

template<typename M>
class InstancedMaterial : public M {

};

class PhongMaterial : public Material {
    typedef std::tuple<int, int> cache_key_t;
    cache_key_t prevKey;
public:
    Shader shader;
    std::optional<Texture> diffuse = Texture::defaultTexture(DefaultTexture::WHITE);
    glm::vec4 color;
    glm::float32 shininess;

    PhongMaterial() : color(1.0f), shininess(25.0f) {}

    GLuint useShaderProgram(Scene &sc, Camera &cam, ProgramArguments &pargs) noexcept override;

    void updateUniforms(Object3D *object, Camera &camera) override;

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
