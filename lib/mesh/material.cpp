#include <material.h>
#include <scene.h>
#include <renderer.h>
#include <lighting.h>
#include <camera.h>
#include <texture.h>

#include <string>
#include <utility>
#include <tuple>
#include <fstream>
#include <regex>

int cg::Material::latest_id = 1;

inline std::string readShaderFile(const char *path) {
    static std::regex pattern(R"(#include\s+("|<)(.*)(>|"))");
    const auto file = cg::readFile(path);
    const auto parent = std::filesystem::u8path(path).parent_path();
    std::smatch matches{};
    auto first = file.begin();
    auto last = file.end();
    std::string output;
    while (std::regex_search(first, last, matches, pattern, std::regex_constants::match_default)) {
        output.append(first, matches[0].first);
        first = matches[0].second;
        auto filename = matches[2].str();
        auto fspath = parent / filename;
        auto importedFile = cg::readFile(fspath);
        output.append(importedFile);
    }
    output.append(first, last);
    return output;
}

GLuint cg::PhongMaterial::useShaderProgram(cg::Scene &sc, cg::Camera &cam, cg::ProgramArguments &pargs) noexcept {
    static std::string phong_vertex = readShaderFile("lib/shaders/phong.vsh");
    static std::string phong_fragment = readShaderFile("lib/shaders/phong.fsh");
    bool _shader_need_update = false;
    if (shader.isNull()) {
        _shader_need_update = true;
    }
    cache_key_t key = std::make_tuple(pargs.pointLightCount, pargs.directionalLightCount);
    if (key != prevKey) {
        _shader_need_update = true;
    }
    prevKey = key;
    // if update is needed, recompile a shader
    if (_shader_need_update) {
        puts(shader.isNull() ? "Recreating phong shader." : "Creating phong shader.");
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
        shader = Shader(vert.data(), frag.data());
    }
    GLuint sp = shader.use();
    if (sp) {
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

        // bind camera origin
        shader.setUniform3f("cameraPosition", cam.worldPosition());
        return sp;
    }
    return 0;
}

void cg::PhongMaterial::updateUniforms(cg::Object3D *object, Camera &camera) {
    GLuint sp = shader.id();
    if (!sp) return;
    auto modelMatrix = object->modelMatrix();
    glUniformMatrix4fv(glGetUniformLocation(sp, "modelMatrix"), 1, GL_FALSE, &modelMatrix[0][0]);
    auto vMatrix = camera.viewMatrix();
    auto pMatrix = camera.projectionMatrix();
    auto mvpMatrix = pMatrix * vMatrix * modelMatrix;

    glUniformMatrix4fv(glGetUniformLocation(sp, "mvpMatrix"), 1, GL_FALSE, &mvpMatrix[0][0]);
}

GLuint cg::StandardMaterial::useShaderProgram(cg::Scene &scene, cg::Camera &camera, cg::ProgramArguments &pargs) {
    static std::string standard_vertex = readShaderFile("lib/shaders/standard.vsh");
    static std::string standard_fragment = readShaderFile("lib/shaders/standard.fsh");
    bool _shader_need_update = false;
    if (shader.isNull()) {
        _shader_need_update = true;
    }
    cache_key_t key = std::make_tuple(pargs.pointLightCount, pargs.directionalLightCount);
    if (key != prevKey) {
        _shader_need_update = true;
    }
    prevKey = key;
    // if update is needed, recompile a shader
    if (_shader_need_update) {
        puts(shader.isNull() ? "Recreating standard shader." : "Creating standard shader.");
        std::string vert = standard_vertex;
        std::string frag = standard_fragment;
        const char *header_tag = "// {headers}";
        auto pos = frag.find(header_tag);
        if (pos != std::string::npos) {
            std::stringstream header;
            header << "#define POINT_LIGHT_COUNT " << pargs.pointLightCount << std::endl;
            header << "#define DIRECTIONAL " << !!pargs.directionalLightCount << std::endl;

            frag.replace(pos, strlen(header_tag), header.str());
        }
        shader = Shader(vert.data(), frag.data());
    }
    GLuint sp = shader.use();
    if (sp) {
        char name[256];
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
        shader.setUniform3f("ambientLight", pargs.ambientColor);

        // albedo
        shader.setUniform1i("albedoMap", 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, albedoMap.has_value() ? albedoMap.value().tex() : 0);

        // metallic
        shader.setUniform1i("metallicMap", 1);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, metallicMap.has_value() ? metallicMap.value().tex() : 0);
        shader.setUniform1f("metallicIntensity", metallicIntensity);

        // roughness
        shader.setUniform1i("roughnessMap", 2);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, roughnessMap.has_value() ? roughnessMap.value().tex() : 0);
        shader.setUniform1f("roughnessIntensity", roughnessIntensity);

        // ambient occlusion
        shader.setUniform1i("aoMap", 3);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, aoMap.has_value() ? aoMap.value().tex() : 0);
        shader.setUniform1f("aoIntensity", aoIntensity);

        // normal
        shader.setUniform1i("normalMap", 4);
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, normalMap.has_value() ? normalMap.value().tex() : 0);

        // bind camera origin
        shader.setUniform3f("cameraPosition", camera.worldPosition());
        return sp;
    }
    return 0;
}

void cg::StandardMaterial::updateUniforms(cg::Object3D *object, cg::Camera &camera) {
    GLuint sp = shader.id();
    if (!sp) return;
    auto modelMatrix = object->modelMatrix();
    shader.setUniformMatrix4("modelMatrix", modelMatrix);
    auto vMatrix = camera.viewMatrix();
    auto pMatrix = camera.projectionMatrix();
    auto mvpMatrix = pMatrix * vMatrix * modelMatrix;
    shader.setUniformMatrix4("mvpMatrix", mvpMatrix);
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
    if (!shader.id()) {
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
    return shader.id();
}

void cg::SkyboxMaterial::updateUniforms(cg::Object3D *object, cg::Camera &camera) {}
