#include <material.h>
#include <scene.h>
#include <renderer.h>
#include <lighting.h>

#include <string>
#include <sstream>
#include <map>


static float TEX_WHITE[] = {1.0f, 1.0f, 1.0f, 1.0f};
static float TEX_BLACK[] = {0.0f, 0.0f, 0.0f, 1.0f};
static float TEX_NORMAL[] = {0.0f, 0.0f, 1.0f, 1.0f};
static float TEX_TRANSPARENT[] = {0.0f, 0.0f, 0.0f, 0.0f};


cg::Texture cg::Texture::defaultTexture(cg::DefaultTexture type) {
    static std::map<cg::DefaultTexture, cg::Texture> defaultTexMap;
    auto it = defaultTexMap.find(type);
    if (it != defaultTexMap.end()) {
        return it->second;
    }

    float *data = nullptr;
    switch (type) {
        case DefaultTexture::WHITE:
            data = TEX_WHITE;
            break;
        case DefaultTexture::BLACK:
            data = TEX_BLACK;
            break;
        case DefaultTexture::DEFAULT_NORMAL:
            data = TEX_NORMAL;
            break;
        case DefaultTexture::TRANSPARENT:
            data = TEX_TRANSPARENT;
            break;
    }

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    // set the texture wrapping/filtering options (on the currently bound texture object)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // load and generate the texture
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGBA, GL_FLOAT, data);
//    glGenerateMipmap(GL_TEXTURE_2D);

    return defaultTexMap.emplace(type, tex).first->second;
}

static std::string readFile(const char *path) {
    std::stringstream ret;
    char buf[1024];
    FILE *f = fopen(path, "rb");
    int n;
    while ((n = fread(buf, 1, 1024, f))) {
        ret.write(buf, n);
    }
    return ret.str();
}


int cg::Material::latest_id = 1;

GLuint cg::PhongMaterial::shaderProgram(cg::Scene &sc, cg::Camera &cam, cg::ProgramArguments &pargs) noexcept {
    std::string phong_vertex = readFile("lib/shader/basic.vsh");
    std::string phong_fragment = readFile("lib/shader/basic.fsh");
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
                    pointLight->worldMatrix() * glm::vec4(pointLight->lookAt() - pointLight->position(), 0.0)),
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
    cache_key_t key = std::make_tuple<size_t, size_t>(point_light_cnt, directional_light_cnt);
    if (key != prevKey) {
        _shader_need_update = true;
    }
    prevKey = key;
    // if update is needed, recompile a shader
    if (_shader_need_update) {
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
        char name[256];
        GLint loc;
        // TODO cache locations
        for (int i = 0; i < point_light_cnt; ++i) {
            snprintf(name, 256, "pointLights[%d].position", point_light_cnt);
            glUniform3fv(glGetUniformLocation(sp, name), 3, &pointLights[i].position[0]);

            snprintf(name, 256, "pointLights[%d].direction", point_light_cnt);
            glUniform3fv(glGetUniformLocation(sp, name), 3, &pointLights[i].direction[0]);

            snprintf(name, 256, "pointLights[%d].color", point_light_cnt);
            glUniform4fv(glGetUniformLocation(sp, name), 4, &pointLights[i].color[0]);
        }
        if (directional_light_cnt) {
            snprintf(name, 256, "directionalLight[%d].position", point_light_cnt);
            glUniform3fv(glGetUniformLocation(sp, name), 3, &dirLights[0].position[0]);

            snprintf(name, 256, "directionalLight[%d].color", point_light_cnt);
            glUniform4fv(glGetUniformLocation(sp, name), 4, &dirLights[0].color[0]);
        }
        glUniform4fv(glGetUniformLocation(sp, "color"), 4, &color[0]);
        glUniform1f(glGetUniformLocation(sp, "shininess"), shininess);

        // bind textures
        glUniform1i(glGetUniformLocation(sp, "diffuse"), 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuse.has_value() ? diffuse.value().tex() : 0);
        return sp;
    }
    return 0;
}
