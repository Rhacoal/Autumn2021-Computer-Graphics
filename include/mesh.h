#ifndef ASSIGNMENT_MESH_H
#define ASSIGNMENT_MESH_H

#include <object3d.h>
#include <cg_fwd.h>
#include <memory>
#include <utility>

namespace cg {
class Mesh : public Object3D {
    std::shared_ptr<Material> _mat;
    std::shared_ptr<Geometry> _geo;
public:
    Mesh(std::shared_ptr<Material> material, std::shared_ptr<Geometry> geometry)
        : _geo(std::move(geometry)), _mat(std::move(material)) {}

    cg::Mesh *isMesh() override { return this; }

    void render(Renderer &re, Scene &sc, Camera &ca) override;
};

class InstancedMesh : public Mesh {
    int count;
public:
    InstancedMesh(int count, std::shared_ptr<Material> material, std::shared_ptr<Geometry> geometry)
        : count(count), Mesh(std::move(material), std::move(geometry)) {}
};
}

#endif //ASSIGNMENT_MESH_H
