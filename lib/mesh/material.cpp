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
    static std::string phong_vertex = readFile("lib/shader/phong.vsh");
    static std::string phong_fragment = readFile("lib/shader/phong.fsh");
    bool _shader_need_update = false;
    if (!shader.id) {
        _shader_need_update = true;
    }
    // construct cache key
    int point_light_cnt = 0;
    int directional_light_cnt = 0;
    // a maximum of 100 point lights are tracked
    struct PointLightStruct {
        glm::vec3 position;
        glm::vec3 direction;
        glm::vec4 color;
    } pointLights[100] = {};
    // only one directional light is tracked
    struct DirectionalLightStruct {
        glm::vec3 direction;
        glm::vec4 color;
    } dirLights[1] = {};
    glm::vec4 ambient_color{};
    for (Light *light : pargs.lights) {
        if (light->isPointLight()) {
            auto pointLight = light->isPointLight();
            pointLights[point_light_cnt++] = PointLightStruct{
                .position = pointLight->localToWorld(pointLight->position()),
                .direction = glm::vec3(
                    pointLight->parentModelMatrix() * glm::vec4(pointLight->lookDir(), 0.0)),
                .color = glm::vec4(pointLight->color(), pointLight->intensity()),
            };
        } else if (light->isDirectionalLight()) {
            directional_light_cnt += 1;
            if (directional_light_cnt == 1) {
                auto dirLight = light->isDirectionalLight();
                dirLights[0] = DirectionalLightStruct{
                    .direction = glm::vec3(dirLight->parentModelMatrix() * glm::vec4(dirLight->lookDir(), 0.0)),
                    .color = glm::vec4(dirLight->color(), dirLight->intensity()),
                };
            }
        } else if (light->isAmbientLight()) {
            ambient_color = glm::vec4(light->color(), light->intensity());
        }
    }
    cache_key_t key = std::make_tuple(point_light_cnt, directional_light_cnt);
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
            header << "#define POINT_LIGHT_COUNT " << point_light_cnt << std::endl;
            header << "#define DIRECTIONAL " << !!directional_light_cnt << std::endl;

            frag.replace(pos, strlen(header_tag), header.str());
        }
        this->shader = Shader(vert.data(), frag.data());
        if (this->shader.id == 0) {
            exit(0);
        }
    }
    GLuint sp = this->shader.id;
    if (sp) {
        glUseProgram(sp);
        char name[256];
        // TODO cache locations
        for (int i = 0; i < point_light_cnt; ++i) {
            snprintf(name, 256, "pointLights[%d].position", point_light_cnt);
            shader.setUniform3f(name, pointLights[i].position);

            snprintf(name, 256, "pointLights[%d].direction", point_light_cnt);
            shader.setUniform3f(name, pointLights[i].direction);

            snprintf(name, 256, "pointLights[%d].color", point_light_cnt);
            shader.setUniform4f(name, pointLights[i].color);
        }
        if (directional_light_cnt) {
            shader.setUniform3f("directionalLight.direction", dirLights[0].direction);
            shader.setUniform4f("directionalLight.color", dirLights[0].color);
        }
        shader.setUniform4f("color", color);
        shader.setUniform1f("shininess", shininess);
        shader.setUniform4f("ambientLight", ambient_color);

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

glm::vec4 homo(glm::vec4 in) {
    return in / in.w;
}

void cg::PhongMaterial::updateUniforms(cg::Object3D *object, Camera &camera) {
    GLuint sp = shader.id;
    if (!sp) return;
    auto modelMatrix = object->modelMatrix();
    glUniformMatrix4fv(glGetUniformLocation(sp, "modelMatrix"), 1, GL_FALSE, &modelMatrix[0][0]);
    auto vMatrix = camera.viewMatrix();
    auto pMatrix = camera.projectionMatrix();
    auto mvpMatrix = pMatrix * vMatrix * modelMatrix;

//    auto zero = mvpMatrix * glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
//    auto x = homo(vMatrix * glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
//    auto y = homo(vMatrix * glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
//    auto z = homo(vMatrix * glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));
//
//    float halfX = 1.0f, halfY = 1.0f, halfZ = 1.0f;
//    float positions[] = {
//        -halfX, -halfY, -halfZ,
//        halfX, -halfY, -halfZ,
//        halfX, halfY, -halfZ,
//        -halfX, halfY, -halfZ,
//        -halfX, -halfY, halfZ,
//        halfX, -halfY, halfZ,
//        halfX, halfY, halfZ,
//        -halfX, halfY, halfZ,
//    };
//    glm::vec4 tet[8];
//    for (int i = 0; i < 8; ++i) {
//        tet[i] = homo(vMatrix * glm::vec4(positions[i * 3], positions[i * 3 + 1], positions[i * 3 + 2], 1.0f));
//        printf("(%.2f, %.2f, %.2f) -> (%.2f, %.2f, %.2f)\n",
//               positions[i * 3], positions[i * 3 + 1], positions[i * 3 + 2],
//               tet[i].x, tet[i].y, tet[i].z);
//    }
//    exit(0);

    glUniformMatrix4fv(glGetUniformLocation(sp, "mvpMatrix"), 1, GL_FALSE, &mvpMatrix[0][0]);
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
