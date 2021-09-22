#ifndef ASSIGNMENT_SCENE_HPP
#define ASSIGNMENT_SCENE_HPP

#include <glm/glm.hpp>
#include <vector>
#include <cgpa_common.hpp>
#include <material.hpp>
#include <memory>

namespace cgpa {
glm::mat4 look_at(glm::vec3 up, glm::vec3 target, glm::vec3 pos);

// 3d objects, y-up
class object3d {
    glm::mat4 _view_matrix;
    glm::mat4 _world_matrix;
    // position in location coordinates
    glm::vec3 _pos{0.0, 0.0, 0.0};
    glm::vec3 _target{0.0, 0.0, -1.0};

    // need update
    bool _need_update = true;
    std::vector<object3d *> _children;

public:
    object3d() {
        update_world_matrix();
    }

    void update_world_matrix() {
        _view_matrix = cgpa::look_at(glm::vec3{0.0, 1.0, 0.0}, _target, _pos);
        _world_matrix = glm::inverse(_view_matrix);
        _need_update = false;
    }

    [[nodiscard]] glm::vec3 position() const {
        return _pos;
    }

    void set_position(glm::vec3 pos) {
        _pos = pos;
        _need_update = true;
    }

    void set_look_at(glm::vec3 target) {
        _target = target;
        _need_update = true;
    }

    [[nodiscard]] glm::vec3 look_at() const {
        return _target;
    }

    void add_child(object3d *obj) {
        _children.push_back(obj);
    }

    glm::vec3 world_to_model(glm::vec3 pos) {
        auto model_pos = _view_matrix * glm::vec4(pos, 1.0);
        return glm::vec3(model_pos.x, model_pos.y, model_pos.z);
    }

    object3d *remove_child(object3d *obj) {
        auto iter = std::find(std::begin(_children), std::end(_children), obj);
        if (iter != _children.end()) {
            _children.erase(iter);
            return obj;
        }
        return nullptr;
    }

    template<typename Renderer, typename Camera>
    void render(Renderer &&renderer, Camera &&camera) {
        for (auto *child : _children) {
            child->render(renderer, camera);
        }
    }

    virtual ~object3d() {
        for (auto *child : _children) {
            delete child;
        }
        _children.clear();
    }
};

class scene : public object3d {
};

class camera : public object3d {
protected:
    glm::mat4 _project;
public:
    [[nodiscard]] const glm::mat4 &projection_matrix() const {
        return _project;
    }
}
};

class perspective_camera : public camera {
public:
    perspective_camera(float fov, float near, float far, float aspect_ratio) {
        update_parameter(fov, near, far, aspect_ratio);
    }

    void update_parameter(float fov, float near, float far, float aspect_ratio) {
        // generate matrix
        glm::mat4 mat;
        // todo: finish this
    }
};

class renderer {
public:
};

#endif //ASSIGNMENT_SCENE_HPP
