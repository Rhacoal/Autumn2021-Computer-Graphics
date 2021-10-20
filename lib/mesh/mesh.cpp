#include <mesh.h>
#include <geometry.h>
#include <renderer.h>
#include <material.h>

void cg::Mesh::render(cg::Renderer &re, cg::Scene &sc, cg::Camera &ca) {
    re.draw(_mat.get(), _geo.get(), this);
}

cg::BoundingBox cg::Mesh::computeBoundingBox() const {
    std::optional<BoundingBox> box;
    _geo->traverseVertices([&](float x, float y, float z) {
        glm::vec4 point{x, y, z, 1.0};
        point = modelMatrix() * point;
        if (box.has_value()) {
            box->addPoint(glm::vec3(point));
        } else {
            box.emplace(glm::vec3(point));
        }
    });
    return box.value();
}

cg::Skybox::Skybox(const char **faces) : _mat(std::make_shared<cg::SkyboxMaterial>()),
                                         _geo(std::make_shared<cg::BoxGeometry>(2.0f, 2.0f, 2.0f, Side::BackSide)) {
    _mat->skybox = CubeTexture::load(faces);
//    _mat->color = glm::vec4(0.0, 0.0, 0.4, 1.0);
}

void cg::Skybox::render(cg::Renderer &re, cg::Scene &sc, cg::Camera &ca) {
    re.draw(_mat.get(), _geo.get(), this);
}
