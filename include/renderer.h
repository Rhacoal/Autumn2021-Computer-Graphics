#ifndef ASSIGNMENT_RENDERER_H
#define ASSIGNMENT_RENDERER_H

#include <cg_fwd.h>
#include <cg_common.h>
#include <object3d.h>

#include <glm/glm.hpp>

#include <vector>

namespace cg {
struct ProgramArguments {
    std::vector<Light *> lights;
};

class Renderer {
    std::vector<std::tuple<Material *, Geometry *>> draw_calls;
public:
    void render(Scene &sc, Camera &cam);

    void draw(Material *mat, Geometry *geo) {
        draw_calls.emplace_back(mat, geo);
    }
};
}

#endif //ASSIGNMENT_RENDERER_H
