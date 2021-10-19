#ifndef ASSIGNMENT_MATERIAL_H
#define ASSIGNMENT_MATERIAL_H

#include <cg_common.h>
#include <cg_fwd.h>
#include <shader.h>
#include <texture.h>

#include <string>
#include <optional>
#include <memory>

namespace cg {

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

    virtual bool isTransparent() const noexcept { return false; };
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

    PhongMaterial() : color(1.0f), shininess(1.0f) {}

    GLuint useShaderProgram(Scene &scene, Camera &camera, ProgramArguments &pargs) noexcept override;

    void updateUniforms(Object3D *object, Camera &camera) override;
};

class SkyboxMaterial : public Material {
public:
    Shader shader;
    std::optional<CubeTexture> skybox;

    SkyboxMaterial() = default;

    GLuint useShaderProgram(Scene &scene, Camera &camera, ProgramArguments &pargs) noexcept override;

    void updateUniforms(Object3D *object, Camera &camera) override;
};

struct StandardMaterial : public Material {
    std::optional<Texture> albedo;
    std::optional<Texture> mra;
    std::optional<Texture> normal;
};
}

#endif //ASSIGNMENT_MATERIAL_H
