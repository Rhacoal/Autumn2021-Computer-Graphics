#include <obj_loader.h>
#include <cg_common.h>
#include <object3d.h>
#include <mesh.h>
#include <geometry.h>
#include <material.h>
#include <application.h>

#include <json.hpp>

#include <vector>
#include <regex>
#include <optional>
#include <functional>
#include <map>
#include <fstream>

using namespace cg;

static std::vector<std::string> split(const std::string &str, const std::regex &delim) {
    // https://stackoverflow.com/a/45204031/14470738
    return std::vector<std::string>{std::sregex_token_iterator{str.begin(), str.end(), delim, -1}, {}};
}

Object3D *toPlaceHolder(nlohmann::json &);

Mesh *toBox(nlohmann::json &);

Material *toMaterial(nlohmann::json &);

Object3D *toObject(nlohmann::json &obj) {
    if (obj["type"] == "placeholder") {
        return toPlaceHolder(obj);
    } else if (obj["type"] == "box") {
        return toBox(obj);
    }
    return nullptr;
};

Mesh *toBox(nlohmann::json &obj) {
    auto &position = obj["position"];
    auto &size = obj["size"];
    auto &rotation = obj["rotation"];
    auto &color = obj["color"];

    auto *material = new PhongMaterial();
    auto *geometry = new BoxGeometry(size[0], size[1], size[2]);

    Mesh *mesh = new Mesh(std::shared_ptr<Material>(material), std::shared_ptr<MeshGeometry>(geometry));
    material->color = glm::vec4{color[0], color[1], color[2], 1.0f};
    if (obj.contains("shininess")) {
        material->shininess = obj["shininess"];
    }
    mesh->setPosition(glm::vec3{position[0], position[1], position[2]});
    mesh->applyRotation(glm::quat{rotation[0], rotation[1], rotation[2], rotation[3]});
    return mesh;
};

Material *toMaterial(nlohmann::json &obj) {
    auto &type = obj["material"];
    const auto loadTexture = [](nlohmann::json &tex, Texture::Encoding defaultEncoding) {
        std::string path = tex["path"];
        float intensity = 1.0f;
        {
            auto it = tex.find("encoding");
            if (it != tex.end()) {
                if (*it == "linear") {
                    defaultEncoding = Texture::Encoding::LINEAR;
                } else if (*it == "srgb") {
                    defaultEncoding = Texture::Encoding::SRGB;
                }
            }
        }
        {
            auto it = tex.find("intensity");
            if (it != tex.end()) {
                intensity = *it;
            }
        }
        return std::make_pair(Application::loadTexture(path.c_str(), defaultEncoding), intensity);
    };
    const auto getTexture = [&](const char *key,
                                Texture::DefaultTexture defaultTexture,
                                Texture::Encoding defaultEncoding = Texture::Encoding::LINEAR) {
        auto it = obj.find(key);
        if (it == obj.end()) {
            return std::make_pair(Texture::defaultTexture(defaultTexture), 1.0f);
        }
        return loadTexture(*it, defaultEncoding);
    };
    if (type == "standard") {
        auto stdMat = new StandardMaterial;
        obj.find("albedo");
        auto albedo = getTexture("albedo", Texture::DefaultTexture::WHITE);
        stdMat->albedoMap = albedo.first;
        stdMat->albedoIntensity = albedo.second;
        auto metallic = getTexture("metallic", Texture::DefaultTexture::WHITE);
        stdMat->metallicMap = metallic.first;
        stdMat->metallicIntensity = metallic.second;
        auto roughness = getTexture("roughness", Texture::DefaultTexture::WHITE);
        stdMat->roughnessMap = roughness.first;
        stdMat->roughnessIntensity = roughness.second;
        auto ao = getTexture("ao", Texture::DefaultTexture::WHITE);
        stdMat->aoMap = ao.first;
        stdMat->aoIntensity = ao.second;
        auto normal = getTexture("normal", Texture::DefaultTexture::DEFAULT_NORMAL);
        stdMat->normalMap = normal.first;
    }
    return nullptr;
}

Object3D *toPlaceHolder(nlohmann::json &obj) {
    auto &position = obj["position"];
    auto &rotation = obj["rotation"];

    Object3D *ob = new Object3D;
    ob->setPosition(glm::vec3{position[0], position[1], position[2]});
    ob->applyRotation(glm::quat{rotation[0], rotation[1], rotation[2], rotation[3]});
    if (obj.contains("children")) {
        for (auto &child : obj["children"]) {
            ob->addChild(toObject(child));
        }
    }
    return ob;
};

cg::Object3D *cg::loadJsonScene(const char *path) {
    auto *root = new Object3D;
    try {
        std::ifstream i(std::filesystem::u8path(path));
        nlohmann::json j;

        i >> j;
        for (auto &obj : j["objects"]) {
            auto *o = toObject(obj);
            if (o) root->addChild(o);
        }
        return root;
    } catch (std::exception &ex) {
        delete root;
        return nullptr;
    }
}
