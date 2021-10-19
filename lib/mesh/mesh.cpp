#include <mesh.h>
#include <geometry.h>
#include <renderer.h>
#include <material.h>

void cg::Mesh::render(cg::Renderer &re, cg::Scene &sc, cg::Camera &ca) {
    re.draw(_mat.get(), _geo.get(), this);
}

cg::Skybox::Skybox(const char **faces) : _mat(std::make_shared<cg::SkyboxMaterial>()),
                                         _geo(std::make_shared<cg::BoxGeometry>(2.0, 2.0, 2.0, Side::BackSide)) {
    _mat->skybox = CubeTexture::load(faces);
//    _mat->color = glm::vec4(0.0, 0.0, 0.4, 1.0);
}

void cg::Skybox::render(cg::Renderer &re, cg::Scene &sc, cg::Camera &ca) {
    re.draw(_mat.get(), _geo.get(), this);
}
