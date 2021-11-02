#ifndef ASSIGNMENT_OBJECT3D_H
#define ASSIGNMENT_OBJECT3D_H

#include <cg_fwd.h>
#include <cg_common.h>

#include <glm/gtc/quaternion.hpp>

#include <vector>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace cg {

glm::mat4 look_at(glm::vec3 up, glm::vec3 target, glm::vec3 pos);

void decompose(const glm::mat4 &mat, glm::vec3 &position, glm::quat &quat, glm::vec3 &scale);

void compose(glm::mat4 &mat, const glm::vec3 &position, const glm::quat &quat, const glm::vec3 &scale);

struct BoundingBox {
    float x0, y0, z0;
    float x1, y1, z1;

    explicit BoundingBox(glm::vec3 vec) {
        x0 = x1 = vec.x;
        y0 = y1 = vec.y;
        z0 = z1 = vec.z;
    }

    explicit BoundingBox(float x, float y, float z) {
        x0 = x1 = x;
        y0 = y1 = y;
        z0 = z1 = z;
    }

    void addPoint(float x, float y, float z) {
        static constexpr auto minmax = [](float &min_v, float &max_v, float v) {
            if (v < min_v) {
                min_v = v;
            } else if (v > max_v) {
                max_v = v;
            }
        };
        minmax(x0, x1, x);
        minmax(y0, y1, y);
        minmax(z0, z1, z);
    }

    void addPoint(glm::vec3 vec) {
        addPoint(vec.x, vec.y, vec.z);
    }

    bool intersect(const BoundingBox &b) const;
};

// 3d objects, y-up
class Object3D {
    inline static int latest_id = 1;
    mutable glm::mat4 _local_matrix;
    mutable glm::mat4 _world_matrix;
    mutable bool _need_update = true;

    // origin in location coordinates
    glm::vec3 _pos{0.0f, 0.0f, 0.0f};
    glm::quat _quat;
    glm::vec3 _scale{1.0f, 1.0f, 1.0f};

    glm::vec3 _up{0.0f, 1.0f, 0.0f};

    std::vector<Object3D *> _children;
    Object3D *_parent = nullptr;
public:
    const int id;

    Object3D() : id(latest_id++) {
        setLocalMatrix(glm::mat4(1.0f));
    }

    Object3D(const Object3D &) = delete;

    Object3D(Object3D &&) = delete;

    void updateWorldMatrix() const {
        compose(_local_matrix, _pos, _quat, _scale);
        if (_parent) {
            _world_matrix = _parent->worldMatrix() * _local_matrix;
        } else {
            _world_matrix = _local_matrix;
        }
    }

    glm::vec3 position() const {
        return _pos;
    }

    void setPosition(glm::vec3 pos) {
        _pos = pos;
        invalidateMatrix();
    }

    void applyRotation(glm::quat quat) {
        _quat = quat * _quat;
        invalidateMatrix();
    }

    void invalidateMatrix(bool force = false) {
        if (!_need_update || force) {
            _need_update = true;
            for (auto *child : _children) {
                child->invalidateMatrix(force);
            }
        }
    }

    void lookAt(glm::vec3 target) {
        glm::vec3 t = target, p = _pos;
        auto lookAtMat = cg::look_at(_up, t, p);
        _quat = glm::conjugate(glm::quat_cast(lookAtMat));
        invalidateMatrix();
    }

    void lookToDir(glm::vec3 dir) {
        lookAt(_pos + dir);
    }

    glm::vec3 lookDir() const {
        if (_need_update) {
            updateWorldMatrix();
        }
        return glm::normalize(glm::vec3(glm::inverse(_local_matrix) * glm::vec4{0.0f, 0.0f, -1.0f, 0.0f}));
    }

    glm::vec3 lookDirWorld() const {
        return glm::normalize(glm::vec3(glm::inverse(worldMatrix()) * glm::vec4{0.0f, 0.0f, -1.0f, 0.0f}));
    }

    glm::vec3 up() const {
        return _up;
    }

    void setUp(glm::vec3 up) {
        _up = glm::normalize(up);
    }

    void setLocalMatrix(const glm::mat4 &local) {
        decompose(local, _pos, _quat, _scale);
        invalidateMatrix();
    }

    void addChild(Object3D *obj) {
        if (obj->_parent == this) return;
        if (obj->_parent) {
            obj->_parent->removeChild(obj);
        }
        obj->_parent = this;
        _children.push_back(obj);
        obj->invalidateMatrix(true);
    }

    const std::vector<Object3D *> &children() const {
        return _children;
    }

    Object3D *parent() const {
        return _parent;
    }

    glm::vec3 localToWorld(glm::vec3 pos) {
        auto model_pos = _world_matrix * glm::vec4(pos, 1.0);
        return glm::vec3(model_pos.x, model_pos.y, model_pos.z) / model_pos.w;
    }

    Object3D *removeChild(Object3D *obj) {
        auto iter = std::find(std::begin(_children), std::end(_children), obj);
        if (iter != _children.end()) {
            (*iter)->_parent = nullptr;
            _children.erase(iter);
            return obj;
        }
        return nullptr;
    }

    glm::mat4 &worldMatrix() const {
        if (_need_update) {
            updateWorldMatrix();
            _need_update = false;
        }
        return _world_matrix;
    }

    glm::mat4 modelMatrix() const {
        return glm::inverse(worldMatrix());
    }

    glm::mat4 parentModelMatrix() const {
        return _parent ? _parent->modelMatrix() : glm::mat4(1.0);
    }

    glm::vec3 worldPosition() const {
        return _parent ? glm::vec3(_parent->modelMatrix() * glm::vec4(_pos, 1.0f)) : _pos;
    }

    virtual cg::Mesh *isMesh() { return nullptr; }

    virtual cg::Light *isLight() { return nullptr; }

    virtual cg::Camera *isCamera() { return nullptr; }

    // to exclude object from ray tracing scene
    virtual bool isBackground() const { return false; }

    /**
     * Submits draw calls to render.
     */
    virtual void render(Renderer &renderer, Scene &scene, Camera &camera) {}

    template<typename T>
    void traverse(T &&func) {
        func(*this);
        for (auto *child : _children) {
            child->traverse(func);
        }
    }

    virtual ~Object3D() {
        for (auto *child : _children) {
            delete child;
        }
        _children.clear();
    }
};
}
#endif //ASSIGNMENT_OBJECT3D_H
