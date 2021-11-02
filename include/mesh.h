#ifndef ASSIGNMENT_MESH_H
#define ASSIGNMENT_MESH_H

#include <cg_fwd.h>
#include <object3d.h>

#include <memory>
#include <utility>
#include <optional>

namespace cg {
class Mesh : public Object3D {
    std::shared_ptr<Material> _mat;
    std::shared_ptr<MeshGeometry> _geo;
public:
    Mesh(std::shared_ptr<Material> material, std::shared_ptr<MeshGeometry> geometry)
        : _geo(std::move(geometry)), _mat(std::move(material)) {}

    cg::Mesh *isMesh() override { return this; }

    MeshGeometry *geometry() {
        return _geo.get();
    }

    Material *material() {
        return _mat.get();
    }

    BoundingBox computeBoundingBox() const;

    void render(Renderer &re, Scene &sc, Camera &ca) override;
};

class Skybox : public Object3D {
    std::shared_ptr<SkyboxMaterial> _mat;
    std::shared_ptr<BoxGeometry> _geo;
public:
    Skybox(const char **faces);

    SkyboxMaterial *skyboxMaterial() {
        return _mat.get();
    }

    void render(Renderer &re, Scene &sc, Camera &ca) override;
};

class InstancedMesh : public Mesh {
    int count;
public:
    InstancedMesh(int count, std::shared_ptr<Material> material, std::shared_ptr<MeshGeometry> geometry)
        : count(count), Mesh(std::move(material), std::move(geometry)) {}
};
}

#endif //ASSIGNMENT_MESH_H
