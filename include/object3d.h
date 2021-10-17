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
    mutable glm::mat4 _world_matrix;
    // position in location coordinates
    glm::vec3 _pos{0.0, 0.0, 0.0};
    glm::vec3 _target{0.0, 0.0, -1.0};

    // need update
    mutable bool _need_update = true;
    std::vector<Object3D *> _children;
    Object3D *_parent = nullptr;
public:
    Object3D() {
        updateWorldMatrix();
    }

    Object3D(const Object3D &) = delete;

    Object3D(Object3D &&) = delete;

    void updateWorldMatrix() const {
        _local_matrix = cg::look_at(glm::vec3{0.0, 1.0, 0.0}, _target, _pos);
        if (_parent) {
            _world_matrix = _parent->_world_matrix * _local_matrix;
        } else {
            _world_matrix = _local_matrix;
        }
        _need_update = false;
    }

    glm::vec3 position() const {
        return _pos;
    }

    void setPosition(glm::vec3 pos) {
        _pos = pos;
        _need_update = true;
    }

    void lookAt(glm::vec3 target) {
        _target = target;
        _need_update = true;
    }

    glm::vec3 lookAt() const {
        return _target;
    }

    void addChild(Object3D *obj) {
        _children.push_back(obj);
    }

    const std::vector<Object3D *> &children() const {
        return _children;
    }

    glm::vec3 localToWorld(glm::vec3 pos) {
        auto model_pos = _world_matrix * glm::vec4(pos, 1.0);
        return glm::vec3(model_pos.x, model_pos.y, model_pos.z) / model_pos.w;
    }

    Object3D *removeChild(Object3D *obj) {
        auto iter = std::find(std::begin(_children), std::end(_children), obj);
        if (iter != _children.end()) {
            _children.erase(iter);
            return obj;
        }
        return nullptr;
    }

    glm::mat4 worldMatrix() const {
        if (_need_update) {
            updateWorldMatrix();
        }
        return _world_matrix;
    }

    virtual cg::Mesh *isMesh() { return nullptr; }

    virtual cg::Light *isLight() { return nullptr; }

    virtual void render(Renderer &re, Scene &sc, Camera &ca) {}

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
