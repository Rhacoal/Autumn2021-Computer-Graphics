#include <mesh.h>
#include <renderer.h>

void cg::Mesh::render(cg::Renderer &re, cg::Scene &sc, cg::Camera &ca) {
    re.draw(_mat.get(), _geo.get(), this);
}
