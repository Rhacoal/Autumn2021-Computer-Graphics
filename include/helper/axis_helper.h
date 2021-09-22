#ifndef ASSIGNMENT_AXIS_HELPER_H
#define ASSIGNMENT_AXIS_HELPER_H

#include <object3d.h>
#include <geometry.h>
#include <material.h>

namespace cg {
class AxisHelperGeometry : public BaseGeometry {
    float _lineWidth;
public:
    AxisHelperGeometry(glm::vec3 x, glm::vec3 y, glm::vec3 z, float lineWidth = 1.0f);

    GLenum glPrimitiveType() const noexcept override {
        return GL_LINES;
    }

    float lineWidth() const noexcept {
        return _lineWidth;
    }

    void setLineWidth(float lineWidth) noexcept {
        _lineWidth = lineWidth;
    }

    void bindVAO(GLuint shaderProgram) override {
        BaseGeometry::bindVAO(shaderProgram);
        glLineWidth(_lineWidth);
    }
};

class AxisHelperMaterial : public Material {
    inline static Shader axisHelperShader{};

public:
    GLuint useShaderProgram(Scene &scene, Camera &camera, ProgramArguments &pargs) override;

    void updateUniforms(Object3D *object, Camera &camera) override;
};

class AxisHelper : public Object3D {
    AxisHelperGeometry axisHelperGeometry;
    AxisHelperMaterial axisHelperMaterial;
public:
    AxisHelper(glm::vec3 x, glm::vec3 y, glm::vec3 z, float lineWidth = 1.0f);

    void render(Renderer &renderer, Scene &scene, Camera &camera) override;
};
}

#endif //ASSIGNMENT_AXIS_HELPER_H
