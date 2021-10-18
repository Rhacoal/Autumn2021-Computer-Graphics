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
        glm::vec3 position;
        glm::vec4 color;
    } dirLights[1] = {};
    for (Light *light : pargs.lights) {
        if (light->isPointLight()) {
            auto pointLight = light->isPointLight();
            pointLights[point_light_cnt++] = PointLightStruct{
                .position = pointLight->localToWorld(pointLight->position()),
                .direction = glm::vec3(
                    pointLight->parentModelToWorldMatrix() * glm::vec4(pointLight->lookDir(), 0.0)),
                .color = glm::vec4(pointLight->color(), pointLight->intensity()),
            };
        } else if (light->isDirectionalLight()) {
            directional_light_cnt += 1;
            if (directional_light_cnt == 1) {
                auto dirLight = light->isDirectionalLight();
                dirLights[0] = DirectionalLightStruct{
                    .position = dirLight->localToWorld(dirLight->position()),
                    .color = glm::vec4(dirLight->color(), dirLight->intensity()),
                };
            }
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
        GLint loc;
        // TODO cache locations
        for (int i = 0; i < point_light_cnt; ++i) {
            snprintf(name, 256, "pointLights[%d].position", point_light_cnt);
            glUniform3fv(glGetUniformLocation(sp, name), 1, &pointLights[i].position[0]);

            snprintf(name, 256, "pointLights[%d].direction", point_light_cnt);
            glUniform3fv(glGetUniformLocation(sp, name), 1, &pointLights[i].direction[0]);

            snprintf(name, 256, "pointLights[%d].color", point_light_cnt);
            glUniform4fv(glGetUniformLocation(sp, name), 1, &pointLights[i].color[0]);
        }
        if (directional_light_cnt) {
            snprintf(name, 256, "directionalLight[%d].position", point_light_cnt);
            glUniform3fv(glGetUniformLocation(sp, name), 1, &dirLights[0].position[0]);

            snprintf(name, 256, "directionalLight[%d].color", point_light_cnt);
            glUniform4fv(glGetUniformLocation(sp, name), 1, &dirLights[0].color[0]);
        }
        glUniform4fv(glGetUniformLocation(sp, "color"), 1, &color[0]);
        glUniform1f(glGetUniformLocation(sp, "shininess"), shininess);

        // bind textures
        glUniform1i(glGetUniformLocation(sp, "diffuse"), 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuse.has_value() ? diffuse.value().tex() : 0);

        // bind camera position
        glUniform4fv(glGetUniformLocation(sp, "cameraPosition"), 1, &cam.worldPosition()[0]);
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
    auto worldMatrix = object->modelToWorldMatrix();
    glUniformMatrix4fv(glGetUniformLocation(sp, "modelMatrix"), 1, GL_FALSE, &worldMatrix[0][0]);
    auto vMatrix = glm::inverse(camera.modelToWorldMatrix());
    auto pMatrix = camera.projectionMatrix();
    // todo remove debug
    auto mvpMatrix = pMatrix * vMatrix * worldMatrix;
    auto zero = mvpMatrix * glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    auto x = homo(mvpMatrix * glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
    auto y = homo(mvpMatrix * glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
    auto z = homo(mvpMatrix * glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));

    float halfX = 0.5f, halfY = 0.5f, halfZ = 0.5f;
    float positions[] = {
        -halfX, -halfY, -halfZ,
        halfX, -halfY, -halfZ,
        halfX, halfY, -halfZ,
        -halfX, halfY, -halfZ,
        -halfX, -halfY, halfZ,
        halfX, -halfY, halfZ,
        halfX, halfY, halfZ,
        -halfX, halfY, halfZ,
    };
    glm::vec4 tet[8];
    for (int i = 0; i < 8; ++i) {
        tet[i] = homo(mvpMatrix * glm::vec4(positions[i * 3], positions[i * 3 + 1], positions[i * 3 + 2], 1.0f));
    }
//    auto mvpMatrix = modelToWorldMatrix;
    glUniformMatrix4fv(glGetUniformLocation(sp, "mvpMatrix"), 1, GL_FALSE, &mvpMatrix[0][0]);
}
