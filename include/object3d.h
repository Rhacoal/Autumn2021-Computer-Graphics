#ifndef ASSIGNMENT_OBJECT3D_H
#define ASSIGNMENT_OBJECT3D_H

#include <glm/glm.hpp>
#include <cg_fwd.h>
#include <cg_common.h>
#include <vector>

namespace cg {

glm::mat4 look_at(glm::vec3 up, glm::vec3 target, glm::vec3 pos);

// 3d objects, y-up
class Object3D {
    mutable glm::mat4 _local_matrix;
    mutable glm::mat4 _model_matrix;
    // position in location coordinates
    glm::vec3 _pos{0.0, 0.0, 0.0};
    glm::vec3 _dir{0.0, 0.0, -1.0};
    glm::vec3 _up{0.0, 1.0, 0.0};

    // need update
    std::vector<Object3D *> _children;
    Object3D *_parent = nullptr;
public:
    Object3D() {
        updateModelToWorldMatrix();
    }

    Object3D(const Object3D &) = delete;

    Object3D(Object3D &&) = delete;

    void updateModelToWorldMatrix() const {
        _local_matrix = glm::inverse(cg::look_at(_up, _dir + _pos, _pos));
        if (_parent) {
            _model_matrix = _parent->modelToWorldMatrix() * _local_matrix;
        } else {
            _model_matrix = _local_matrix;
        }
    }

    glm::vec3 position() const {
        return _pos;
    }

    void setPosition(glm::vec3 pos) {
        _pos = pos;
    }

    void lookAt(glm::vec3 target) {
        _dir = glm::normalize(target - _pos);
    }

    glm::vec3 lookDir() const {
        return _dir;
    }

    glm::vec3 up() const {
        return _up;
    }

    void setUp(glm::vec3 up) {
        _up = glm::normalize(up);
    }

    void addChild(Object3D *obj) {
        if (obj->_parent == this) return;
        if (obj->_parent) {
            obj->_parent->removeChild(obj);
        }
        obj->_parent = this;
        _children.push_back(obj);
    }

    const std::vector<Object3D *> &children() const {
        return _children;
    }

    glm::vec3 localToWorld(glm::vec3 pos) {
        auto model_pos = _model_matrix * glm::vec4(pos, 1.0);
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

    glm::mat4 modelToWorldMatrix() const {
        updateModelToWorldMatrix();
        return _model_matrix;
    }

    glm::mat4 parentModelToWorldMatrix() const {
        return _parent ? _parent->modelToWorldMatrix() : glm::mat4(1.0);
    }

    glm::vec3 worldPosition() const {
        return _parent ? glm::vec3(_parent->modelToWorldMatrix() * glm::vec4(_pos, 1.0f)) : _pos;
    }

    virtual cg::Mesh *isMesh() { return nullptr; }

    virtual cg::Light *isLight() { return nullptr; }

    /**
     * Submits draw calls to render.
     */
    virtual void render(Renderer &renderer, Scene &scene, Camera &camera) {}

    template<typename T>
    void traverse(const T &func) {
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
