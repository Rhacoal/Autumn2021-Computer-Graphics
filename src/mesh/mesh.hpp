#ifndef ASSIGNMENT_MESH_HPP
#define ASSIGNMENT_MESH_HPP

#include "geometry.hpp"
#include "material.hpp"
#include <variant>
#include <string>
#include <map>
#include <GL/glew.h>

namespace cgpa {
using uniform_value = std::variant<float>;

template<typename G, typename M>
class mesh {
    G *geometry;
    M *material;
public:
    typedef G geometry_type;
    typedef M material_type;

    mesh(G *geometry, M *material) : geometry(geometry), material(material) {}

    void render() {

    }
};
}

#endif //ASSIGNMENT_MESH_HPP
