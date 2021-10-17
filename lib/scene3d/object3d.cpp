#include <object3d.h>
#include <glm/gtc/matrix_transform.hpp>

glm::mat4 cg::look_at(glm::vec3 up, glm::vec3 target, glm::vec3 pos) {
//    auto dir = pos - target;
//    auto z = glm::normalize(dir);
//    auto x = glm::cross(up, z);
//    auto y = glm::cross(z, x);
//
//    glm::mat4 translation{
//        glm::vec4{1.0, 0.0, 0.0, -pos.x},
//        glm::vec4{0.0, 1.0, 0.0, -pos.y},
//        glm::vec4{0.0, 0.0, 1.0, -pos.z},
//        glm::vec4{0.0, 0.0, 0.0, 1.0},
//    };
//    glm::mat4 rotation{
//        glm::vec4{x, 0.0},
//        glm::vec4{y, 0.0},
//        glm::vec4{z, 0.0},
//        glm::vec4{0.0, 0.0, 0.0, 1.0},
//    };
//
    return glm::lookAt(pos, target, up);
}
