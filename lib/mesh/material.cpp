#include <material.h>
#include <scene.h>
#include <renderer.h>
#include <lighting.h>
#include <camera.h>

#include <string>
#include <sstream>
#include <utility>
#include <tuple>
#include <texture.h>

static std::string readFile(const char *path) {
    std::stringstream ret;
    char buf[1024];
    FILE *f = fopen(path, "rb");
    size_t n;
    while ((n = fread(buf, 1, 1024, f))) {
        ret.write(buf, static_cast<std::streamsize>(n));
    }
    fclose(f);
    return ret.str();
}

int cg::Material::latest_id = 1;

GLuint cg::PhongMaterial::useShaderProgram(cg::Scene &sc, cg::Camera &cam, cg::ProgramArguments &pargs) noexcept {
    static std::string phong_vertex = readFile("lib/shaders/phong.vsh");
    static std::string phong_fragment = readFile("lib/shaders/phong.fsh");
    bool _shader_need_update = false;
    if (!shader.id) {
        _shader_need_update = true;
    }
    cache_key_t key = std::make_tuple(pargs.pointLightCount, pargs.directionalLightCount);
    if (key != prevKey) {
        _shader_need_update = true;
    }
    prevKey = key;
    // if update is needed, recompile a shader
    if (_shader_need_update) {
        puts("recreating shader.");
        std::string vert = phong_vertex;
        std::string frag = phong_fragment;
        const char *header_tag = "// {headers}";
        auto pos = frag.find(header_tag);
        if (pos != std::string::npos) {
            std::stringstream header;
            header << "#define POINT_LIGHT_COUNT " << pargs.pointLightCount << std::endl;
            header << "#define DIRECTIONAL " << !!pargs.directionalLightCount << std::endl;

            frag.replace(pos, strlen(header_tag), header.str());
        }
        this->shader = Shader(vert.data(), frag.data());
    }
    GLuint sp = this->shader.id;
    if (sp) {
        glUseProgram(sp);
        char name[256];
        // TODO cache locations
        for (int i = 0; i < pargs.pointLightCount; ++i) {
            snprintf(name, 256, "pargs.pointLights[%d].position", pargs.pointLightCount);
            shader.setUniform3f(name, pargs.pointLights[i].position);

            snprintf(name, 256, "pargs.pointLights[%d].direction", pargs.pointLightCount);
            shader.setUniform3f(name, pargs.pointLights[i].direction);

            snprintf(name, 256, "pargs.pointLights[%d].color", pargs.pointLightCount);
            shader.setUniform3f(name, pargs.pointLights[i].color);
        }
        if (pargs.directionalLightCount) {
            shader.setUniform3f("directionalLight.direction", pargs.directionalLights[0].direction);
            shader.setUniform3f("directionalLight.color", pargs.directionalLights[0].color);
        }
        shader.setUniform4f("color", color);
        shader.setUniform1f("shininess", shininess);
        shader.setUniform3f("ambientLight", pargs.ambientColor);

        // bind textures
        shader.setUniform1i("diffuse", 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuse.has_value() ? diffuse.value().tex() : 0);

        // bind camera position
        shader.setUniform3f("cameraPosition", cam.worldPosition());
        return sp;
    }
    return 0;
}

void cg::PhongMaterial::updateUniforms(cg::Object3D *object, Camera &camera) {
    GLuint sp = shader.id;
    if (!sp) return;
    auto modelMatrix = object->modelMatrix();
    glUniformMatrix4fv(glGetUniformLocation(sp, "modelMatrix"), 1, GL_FALSE, &modelMatrix[0][0]);
    auto vMatrix = camera.viewMatrix();
    auto pMatrix = camera.projectionMatrix();
    auto mvpMatrix = pMatrix * vMatrix * modelMatrix;

    glUniformMatrix4fv(glGetUniformLocation(sp, "mvpMatrix"), 1, GL_FALSE, &mvpMatrix[0][0]);
}

GLuint cg::StandardMaterial::useShaderProgram(cg::Scene &scene, cg::Camera &camera, cg::ProgramArguments &pargs) {

}

void cg::StandardMaterial::updateUniforms(cg::Object3D *object, cg::Camera &camera) {
}

const char *skybox_vert = R"(
#version 330 core
layout (location = 0) in vec3 position;
out vec3 texCoord;
uniform mat4 vpMatrix;
void main() {
    texCoord = position;
    vec4 pos = vpMatrix * vec4(position, 1.0);
    gl_Position = pos.xyww;
}
)";

const char *skybox_frag = R"(
#version 330 core
out vec4 fragColor;
in vec3 texCoord;
uniform samplerCube skybox;
void main() {
    fragColor = texture(skybox, texCoord);
//    fragColor = texture(skybox, vec3(0.5, 0.5, 0.2));
//    fragColor = vec4(1.0);
}
)";

GLuint cg::SkyboxMaterial::useShaderProgram(Scene &scene, Camera &camera, ProgramArguments &pargs) noexcept {
    if (!shader.id) {
        shader = Shader(skybox_vert, skybox_frag);
    }
    shader.use();
    shader.setUniform1i("skybox", 0);
    auto v0Matrix = camera.viewMatrix();
    auto vMatrix = glm::mat4(glm::mat3(camera.viewMatrix()));
    auto pMatrix = camera.projectionMatrix();
    auto vpMatrix = pMatrix * vMatrix;
    shader.setUniformMatrix4("vpMatrix", vpMatrix);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skybox.has_value() ? skybox->tex() : 0);
    return shader.id;
}

void cg::SkyboxMaterial::updateUniforms(cg::Object3D *object, cg::Camera &camera) {}
