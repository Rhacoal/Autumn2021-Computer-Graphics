#include <object3d.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

glm::mat4 cg::look_at(glm::vec3 up, glm::vec3 target, glm::vec3 pos) {
    return glm::lookAt(pos, target, up);
}

void cg::decompose(const glm::mat4 &mat, glm::vec3 &position, glm::quat &quat, glm::vec3 &scale) {
    // from three.js
    float sx = glm::length(glm::vec3(mat[0]));
    float sy = glm::length(glm::vec3(mat[1]));
    float sz = glm::length(glm::vec3(mat[2]));

    // if dematrmine is negative, we need to invert one scale
    float det = glm::determinant(mat);
    if (det < 0) sx = -sx;
    scale = glm::vec3(sx, sy, sz);

    position = -glm::vec3(mat[3]);

    // scale the rotation part
    glm::mat3 rotation = glm::mat3{
        glm::vec3(mat[0]) / sx,
        glm::vec3(mat[1]) / sy,
        glm::vec3(mat[2]) / sz,
    };

    quat = glm::quat_cast(rotation);
}

void cg::compose(glm::mat4 &mat, const glm::vec3 &position, const glm::quat &quat, const glm::vec3 &scale) {
    auto rot = glm::toMat4(quat);
    mat = glm::scale(glm::translate(glm::mat4(1.0f), -position) * rot, scale);
}
