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
    static constexpr int MAX_POINT_LIGHT_COUNT = 100;
    static constexpr int MAX_DIR_LIGHT_COUNT = 100;

    // a maximum of 100 point lights are tracked
    int pointLightCount;
    struct PointLightStruct {
        glm::vec3 position;
        glm::vec3 direction;
        glm::vec3 color;
    } pointLights[MAX_POINT_LIGHT_COUNT] = {};

    // a maximum of 100 directional lights is tracked
    int directionalLightCount;
    struct DirectionalLightStruct {
        glm::vec3 direction;
        glm::vec3 color;
    } directionalLights[MAX_DIR_LIGHT_COUNT] = {};

    glm::vec3 ambientColor{};

    void clear() {
        lights.clear();
        pointLightCount = 0;
        directionalLightCount = 0;
        ambientColor = glm::vec3{0.0f};
    }
};

class Renderer {
    std::vector<std::tuple<Material *, Geometry *, Object3D *>> draw_calls;
public:
    void render(Scene &sc, Camera &cam);

    void draw(Material *mat, Geometry *geo, Object3D *obj) {
        draw_calls.emplace_back(mat, geo, obj);
    }
};
}

#endif //ASSIGNMENT_RENDERER_H
