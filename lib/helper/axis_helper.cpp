#include <helper/axis_helper.h>
#include <renderer.h>
#include <camera.h>

cg::AxisHelperGeometry::AxisHelperGeometry(glm::vec3 x, glm::vec3 y, glm::vec3 z, float lineWidth)
    : _lineWidth(lineWidth) {
    float positions[6 * 3] = {
        0.0f, 0.0f, 0.0f, x.x, x.y, x.z,    // x-axis
        0.0f, 0.0f, 0.0f, y.x, y.y, y.z,    // y-axis
        0.0f, 0.0f, 0.0f, z.x, z.y, z.z,    // z-axis
    };
    float colors[6 * 3] = {
        1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
    };
    addAttribute("position", positions, 3);
    addAttribute("color", colors, 3);
}

static constexpr const char *vertex = R"(
#version 330 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;
uniform mat4 mvpMatrix;
out vec3 vColor;
void main() {
    vec4 pos = mvpMatrix * vec4(position, 1.0);
    pos.z = -pos.w;
    gl_Position = pos;
    vColor = color;
}
)";

static constexpr const char *fragment = R"(
#version 330 core
in vec3 vColor;
out vec4 fragColor;
void main() {
    fragColor = vec4(vColor, 1.0);
}
)";

void cg::AxisHelperMaterial::updateUniforms(Object3D *object, Camera &camera) {
    axisHelperShader.setUniformMatrix4("mvpMatrix",
        object->modelMatrix() * camera.projectionMatrix() * camera.viewMatrix());
}

GLuint cg::AxisHelperMaterial::useShaderProgram(Scene &scene, Camera &camera, ProgramArguments &pargs) {
    if (axisHelperShader.isNull()) {
        axisHelperShader = Shader(vertex, fragment);
    }
    return axisHelperShader.use();
}

cg::AxisHelper::AxisHelper(glm::vec3 x, glm::vec3 y, glm::vec3 z, float lineWidth)
    : axisHelperGeometry(x, y, z, lineWidth), axisHelperMaterial() {
}

void cg::AxisHelper::render(Renderer &renderer, Scene &scene, Camera &camera) {
    renderer.draw(&axisHelperMaterial, &axisHelperGeometry, this);
}
