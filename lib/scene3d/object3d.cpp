#include <object3d.h>
#include <glm/gtc/matrix_transform.hpp>

glm::mat4 cg::look_at(glm::vec3 up, glm::vec3 target, glm::vec3 pos) {
    return glm::lookAt(pos, target, up);
}
