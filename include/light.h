#ifndef ASSIGNMENT_LIGHT_H
#define ASSIGNMENT_LIGHT_H

#include <scene.h>

namespace cg {
class Light : public Object3D {
    glm::vec3 _color;
    float _intensity;
public:
    explicit Light(glm::vec3 color, float intensity = 1.0) : _color(color), _intensity(intensity) {}

    Light *isLight() override { return this; }

    virtual PointLight *isPointLight() { return nullptr; }

    virtual DirectionalLight *isDirectionalLight() { return nullptr; }

    virtual AmbientLight *isAmbientLight() { return nullptr; }

    glm::vec3 color() const {
        return _color;
    }

    float intensity() const {
        return _intensity;
    }

    void setColor(const glm::vec3 &color) {
        this->_color = color;
    }

    void setIntensity(float intensity) {
        this->_intensity = intensity;
    }
};

class PointLight : public Light {
public:
    explicit PointLight(glm::vec3 color, float intensity = 1.0) : Light(color, intensity) {}

    PointLight *isPointLight() override { return this; }
};

class DirectionalLight : public Light {
public:
    explicit DirectionalLight(glm::vec3 direction, glm::vec3 color, float intensity = 1.0) : Light(color, intensity) {
        setPosition(direction);
        lookAt(glm::vec3(0.0f));
    }

    DirectionalLight *isDirectionalLight() override { return this; }
};

class AmbientLight : public Light {
public:
    explicit AmbientLight(glm::vec3 color, float intensity = 1.0) : Light(color, intensity) {}

    AmbientLight *isAmbientLight() override { return this; }
};
}

#endif //ASSIGNMENT_LIGHT_H
