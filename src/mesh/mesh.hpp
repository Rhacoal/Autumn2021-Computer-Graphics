#ifndef ASSIGNMENT_MESH_HPP
#define ASSIGNMENT_MESH_HPP

#include <material.hpp>
#include <scene.hpp>
#include <geometry.hpp>
#include <memory>

namespace cgpa {
template<typename material_type>
class instanced_material {
public:
    typedef material_traits<material_type> base_material_traits;
};

template<typename material_type, typename geometry_type>
class instanced_mesh : public object3d {
    int count;
    std::shared_ptr<material_type> material;
    std::shared_ptr<geometry_type> geometry;
public:
    instanced_mesh(int count, std::shared_ptr<material_type> material, std::shared_ptr<geometry_type> geometry)
        : count(count), geometry(geometry), material(material) {}
};

template<typename material_type, typename geometry_type>
class mesh : public object3d {
    std::shared_ptr<material_type> material;
    std::shared_ptr<geometry_type> geometry;

    mesh(std::shared_ptr<material_type> material, std::shared_ptr<geometry_type> geometry)
        : geometry(geometry), material(material) {}

private:
    void render(gl_context *context) override {
        // draw self

        // bind VAO

        // draw children
        object3d::render(context);
    }
};
}
#endif //ASSIGNMENT_MESH_HPP
