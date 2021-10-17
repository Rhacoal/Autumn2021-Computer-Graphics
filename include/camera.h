#ifndef ASSIGNMENT_CAMERA_H
#define ASSIGNMENT_CAMERA_H

#include <object3d.h>

#include <glm/glm.hpp>
#include <cg_fwd.h>

namespace cg {
class Camera : public Object3D {
protected:
    glm::mat4 _project;
public:
    const glm::mat4 &projectionMatrix() const {
        return _project;
    }
};

class PerspectiveCamera : public Camera {
protected:
    float _fov, _near, _far, _aspect_ratio, _zoom = 1.0f;

    static void
    makePerspective(glm::mat4 &mat, float left, float right, float top, float bottom, float near, float far) {
        float x = 2 * near / (right - left);
        float y = 2 * near / (top - bottom);

        float a = (right + left) / (right - left);
        float b = (top + bottom) / (top - bottom);
        float c = -(far + near) / (far - near);
        float d = -2 * far * near / (far - near);

        mat = glm::mat4(
            x, 0.0, a, 0.0,
            0.0, y, b, 0.0,
            0.0, 0.0, c, d,
            0.0, 0.0, -1.0, 0.0
        );
    }

public:
    PerspectiveCamera(float fov, float near, float far, float aspect_ratio) :
        _fov(fov), _near(near), _far(far), _aspect_ratio(aspect_ratio) {
        updateParameter(fov, near, far, aspect_ratio);
    }

    void updateParameter(float fov, float near, float far, float aspect_ratio) {
        this->_fov = fov;
        this->_near = near;
        this->_far = far;
        this->_aspect_ratio = aspect_ratio;
        updateMatrix();
    }

    void updateMatrix() {
        float top = _near * tan(math::pi() * 0.5f * _fov) / _zoom;
        float height = 2.0f * top;
        float width = _aspect_ratio * height;
        float left = -0.5f * width;

        float right = left + width;
        float bottom = top - height;

        makePerspective(_project, left, right, top, bottom, _near, _far);
    }
};

class OrthographicCamera : public Camera {
public:
    // TODO finish this
};
}

#endif //ASSIGNMENT_CAMERA_H
