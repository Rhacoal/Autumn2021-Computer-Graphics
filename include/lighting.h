#ifndef ASSIGNMENT_LIGHTING_H
#define ASSIGNMENT_LIGHTING_H

#include <scene.h>

namespace cg {
class Light : public Object3D {
    glm::vec3 _color;
    float _intensity;
public:
    Light(glm::vec3 color, float intensity = 1.0) : _color(color), _intensity(intensity) {}

    cg::Light *isLight() override { return this; }

    virtual cg::PointLight *isPointLight() { return nullptr; }

    virtual cg::DirectionalLight *isDirectionalLight() { return nullptr; }

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
    PointLight(glm::vec3 color, float intensity = 1.0) : Light(color, intensity) {}

    cg::PointLight *isPointLight() override { return this; }
};

class DirectionalLight : public Light {
public:
    DirectionalLight(glm::vec3 color, float intensity = 1.0) : Light(color, intensity) {}

    cg::DirectionalLight *isDirectionalLight() override { return this; }
};
}

#endif //ASSIGNMENT_LIGHTING_H
