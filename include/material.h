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
    glm::vec4 color{1.0f};
    glm::vec4 emission{0.0f};

    Material() : id(latest_id++) {}

    Material(const Material &) = delete;

    Material(Material &&) = delete;

    virtual bool canInstance() const { return false; }

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

class PhongMaterial : public Material {
    typedef std::tuple<int, int> cache_key_t;
    cache_key_t prevKey;
public:
    Shader shader;
    std::optional<Texture> diffuse = Texture::defaultTexture(Texture::DefaultTexture::WHITE);
    glm::float32 shininess;

    PhongMaterial() : shininess(1.0f) {}

    bool canInstance() const override { return true; }

    GLuint useShaderProgram(Scene &scene, Camera &camera, ProgramArguments &pargs) noexcept override;

    void updateUniforms(Object3D *object, Camera &camera) override;
};

class StandardMaterial : public Material {
    typedef std::tuple<int, int> cache_key_t;
    cache_key_t prevKey;
public:
    Shader shader;
    std::optional<Texture> albedoMap;
    float albedoIntensity = 1.0f;
    std::optional<Texture> metallicMap;
    float metallicIntensity = 1.0f;
    std::optional<Texture> roughnessMap;
    float roughnessIntensity = 1.0f;
    std::optional<Texture> aoMap;
    float aoIntensity = 1.0f;
    std::optional<Texture> normalMap;
    std::optional<CubeTexture> envMap;

    StandardMaterial() = default;

    GLuint useShaderProgram(Scene &scene, Camera &camera, ProgramArguments &pargs) override;

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
}

#endif //ASSIGNMENT_MATERIAL_H
