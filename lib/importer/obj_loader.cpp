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
#include <format>

using namespace cg;

static std::vector<std::string> split(const std::string &str, const std::regex &delim) {
    // https://stackoverflow.com/a/45204031/14470738
    return std::vector<std::string>{std::sregex_token_iterator{str.begin(), str.end(), delim, -1}, {}};
}

template<typename T>
T getOrDefault(nlohmann::json &obj, const std::string &key, const T &value) {
    return obj.value(key, value);
}

template<>
glm::vec3 getOrDefault<glm::vec3>(nlohmann::json &obj, const std::string &key, const glm::vec3 &value) {
    auto it = obj.find(key);
    if (it == obj.end()) {
        return value;
    }
    auto &val = *it;
    return glm::vec3{val[0], val[1], val[2]};
}

template<>
glm::vec4 getOrDefault<glm::vec4>(nlohmann::json &obj, const std::string &key, const glm::vec4 &value) {
    auto it = obj.find(key);
    if (it == obj.end()) {
        return value;
    }
    auto &val = *it;
    return glm::vec4{val[0], val[1], val[2], val[3]};
}

template<>
glm::quat getOrDefault<glm::quat>(nlohmann::json &obj, const std::string &key, const glm::quat &value) {
    auto it = obj.find(key);
    if (it == obj.end()) {
        return value;
    }
    auto &val = *it;
    return glm::quat{val[0], val[1], val[2], val[3]};
}


struct ObjLoader {
    std::map<std::string, int> nameIdMap;
    std::map<int, std::shared_ptr<Material>> mtlMap;

    std::shared_ptr<Material> getMaterial(const std::string &str) {
        auto nameIt = nameIdMap.find(str);
        if (nameIt == nameIdMap.end()) {
            return nullptr;
        }
        auto mtlIt = mtlMap.find(nameIt->second);
        return mtlIt == mtlMap.end() ? nullptr : mtlIt->second;
    }

    std::shared_ptr<Material> saveMaterial(const std::string &str, Material *mtl) {
        nameIdMap.emplace(str, mtl->id);
        auto[it, _] = mtlMap.emplace(mtl->id, mtl);
        return it->second;
    }

    static MeshGeometry *loadGeometryFromFile(const std::string &pathStr) {
        auto path = std::filesystem::u8path(pathStr);
        std::ifstream in(path);
        std::string line;
        std::getline(in, line);
        while (std::getline(in, line)) {
            std::stringstream ss{line};
            // TODO finish this! fatal!!!
        }
        return nullptr;
    }

    MeshGeometry *toMeshGeometry(nlohmann::json &obj) {
        auto type = obj["type"];
        if (type == "box") {
            auto size = getOrDefault(obj, "size", glm::vec3(1.0f));
            return new BoxGeometry(size[0], size[1], size[2]);
        } else if (type == "sphere") {
            auto radius = getOrDefault(obj, "radius", 1.0f);
            return new SphereGeometry(radius, 50, 50);
        } else if (type == "file") {
            return loadGeometryFromFile(obj["file"]);
        } else {
            throw std::runtime_error(std::format("unsupported geometry type: {}", type));
        }
    }

    Object3D *toObject(nlohmann::json &obj) {
        if (obj.value("isPlaceholder", false)) {
            return toPlaceHolder(obj);
        } else {
            return toMesh(obj);
        }
    };

    Mesh *toMesh(nlohmann::json &obj) {
        auto position = getOrDefault(obj, "position", glm::vec3(0.0f));
        auto rotation = getOrDefault(obj, "rotation", glm::quat{1.0f, 0.0f, 0.0f, 0.0f});
        auto color = getOrDefault(obj, "color", glm::vec4{1.0f, 1.0f, 1.0f, 1.0f});

        auto geometry = toMeshGeometry(obj["geometry"]);
        auto *material = new PhongMaterial();

        Mesh *mesh = new Mesh(std::shared_ptr<Material>(material), std::shared_ptr<MeshGeometry>(geometry));
        material->color = color;
        if (obj.contains("shininess")) {
            material->shininess = obj["shininess"];
        }
        mesh->setPosition(glm::vec3{position[0], position[1], position[2]});
        mesh->applyRotation(rotation);
        return mesh;
    };

    Material *toMaterial(nlohmann::json &obj) {
        auto &type = obj["type"];
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
            for (auto &child: obj["children"]) {
                ob->addChild(toObject(child));
            }
        }
        return ob;
    };

    cg::Object3D *load(nlohmann::json &j) {
        Object3D *root = new Object3D;
        try {
            for (auto &mtlObj: j["materials"]) {
                auto *mtl = toMaterial(mtlObj);
            }
            for (auto &obj: j["objects"]) {
                auto *o = toObject(obj);
                if (o) root->addChild(o);
            }
        } catch (std::exception &) {
            delete root;
            root = nullptr;
        }
        return root;
    }
};

cg::Object3D *cg::loadJsonScene(const char *path) {
    try {
        std::ifstream i(std::filesystem::u8path(path));
        nlohmann::json j;
        i >> j;
        return ObjLoader{}.load(j);
    } catch (std::exception &ex) {
        return nullptr;
    }
}
