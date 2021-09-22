#include <obj_loader.h>
#include <cg_common.h>
#include <object3d.h>
#include <mesh.h>
#include <geometry.h>
#include <material.h>
#include <light.h>
#include <application.h>

#include <json.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <vector>
#include <regex>
#include <optional>
#include <functional>
#include <map>
#include <iostream>
#include <fstream>
#include <variant>

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
glm::vec2 getOrDefault<glm::vec2>(nlohmann::json &obj, const std::string &key, const glm::vec2 &value) {
    auto it = obj.find(key);
    if (it == obj.end()) {
        return value;
    }
    auto &val = *it;
    return glm::vec2{val[0], val[1]};
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
    if (val.size() > 3) {
        return glm::vec4{val[0], val[1], val[2], val[3]};
    } else {
        return glm::vec4{val[0], val[1], val[2], 1.0f};
    }
}

template<>
glm::quat getOrDefault<glm::quat>(nlohmann::json &j, const std::string &key, const glm::quat &value) {
    auto it = j.find(key);
    if (it == j.end()) {
        return value;
    }
    auto &val = *it;
    return glm::quat{val[0], val[1], val[2], val[3]};
}


struct ObjLoader {
    std::filesystem::path rootPath;
    std::map<std::string, int> nameIdMap;
    std::map<int, std::shared_ptr<Material>> mtlMap;

    std::shared_ptr<Material> getMaterial(const std::string &str) const {
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

    MeshGeometry *loadGeometryFromFile(const std::string &path) const {
        Assimp::Importer importer;

        auto fspath = rootPath / std::filesystem::u8path(path);
        auto fss = fspath.u8string();
        const aiScene *scene = importer.ReadFile(std::string(reinterpret_cast<char*>(fss.data())),
            aiProcess_GenNormals |
            aiProcess_CalcTangentSpace |
            aiProcess_Triangulate |
            aiProcess_SortByPType);

        if (scene == nullptr) {
            std::cerr << "Failed to load model: " << fspath.string() << std::endl;
            return nullptr;
        }
        if (!scene->mNumMeshes) {
            std::cerr << "Invalid model file: " << fspath.string() << std::endl;
            return nullptr;
        }
        auto mesh = scene->mMeshes[0];
        std::vector<float> position;
        std::vector<float> normal;
        std::vector<uint32_t> indices;
        const auto pushVector3 = [](std::vector<float> &arr, const aiVector3D &vec3) {
            arr.push_back(vec3.x);
            arr.push_back(vec3.y);
            arr.push_back(vec3.z);
        };
        for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
            pushVector3(position, mesh->mVertices[i]);
            pushVector3(normal, mesh->mNormals[i]);
        }
        for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
            const auto &face = mesh->mFaces[i];
            indices.push_back(face.mIndices[0]);
            indices.push_back(face.mIndices[1]);
            indices.push_back(face.mIndices[2]);
        }
        auto geometry = new MeshGeometry;
        geometry->addAttribute("position", position, 3);
        geometry->addAttribute("normal", normal, 3);
        geometry->addIndices(indices);
        return geometry;
    }

    MeshGeometry *toMeshGeometry(nlohmann::json &j) const {
        std::string type = j["type"];
        if (type == "box") {
            auto size = getOrDefault(j, "size", glm::vec3(1.0f));
            return new BoxGeometry(size[0], size[1], size[2]);
        } else if (type == "sphere") {
            auto radius = getOrDefault(j, "radius", 1.0f);
            return new SphereGeometry(radius, 50, 50);
        } else if (type == "plane") {
            auto size = getOrDefault(j, "size", glm::vec2(1.0f));
            return new PlaneGeometry(size.x, size.y);
        } else if (type == "file") {
            return loadGeometryFromFile(j["file"]);
        } else {
            throw std::runtime_error("unsupported geometry type: " + type);
        }
    }

    Object3D *toObject(nlohmann::json &j) const {
        if (j.value("isPlaceholder", false)) {
            return toPlaceHolder(j);
        } else {
            return toMesh(j);
        }
    };

    Mesh *toMesh(nlohmann::json &j) const {
        auto position = getOrDefault(j, "position", glm::vec3(0.0f));
        auto scale = getOrDefault(j, "scale", glm::vec3(1.0f));
        auto rotation = getOrDefault(j, "rotation", glm::quat{1.0f, 0.0f, 0.0f, 0.0f});
        auto color = getOrDefault(j, "color", glm::vec4{1.0f, 1.0f, 1.0f, 1.0f});

        auto *geometry = toMeshGeometry(j["geometry"]);
        if (!geometry) {
            std::cerr << "Failed to load mesh: " << j["geometry"] << std::endl;
            return nullptr;
        }
        auto mtlJson = j["material"];
        auto material = mtlJson.is_string() ? getMaterial(mtlJson) : std::shared_ptr<Material>(readMaterial(mtlJson));
        if (!material) {
            delete geometry;
            std::cerr << "Failed to load material: " << mtlJson << std::endl;
            return nullptr;
        }

        Mesh *mesh = new Mesh(std::move(material), std::shared_ptr<MeshGeometry>(geometry));
        mesh->setPosition(glm::vec3{position[0], position[1], position[2]});
        mesh->setScale(scale);
        mesh->applyRotation(rotation);

        if (getOrDefault(j, "isLight", false)) {
            auto pointLight = new PointLight(mesh->material()->emission, 1.0f);
            pointLight->setPosition(glm::vec3(0.0f, 0.001f, 0.0f));
            mesh->addChild(pointLight);
        }
        auto bb = mesh->computeBoundingBox();
        return mesh;
    };

    template<typename T>
    auto loadTexture(const char *textureKey, const char *intensityKey, nlohmann::json &mat,
                     Texture::Encoding defaultEncoding, Texture::DefaultTexture defaultTexture) const {
        auto texIt = mat.find(textureKey);
        if (texIt == mat.end()) {
            return std::make_pair(Texture::defaultTexture(defaultTexture), T{1.0f});
        }
        auto &tex = *texIt;
        auto pathIt = tex.find("texture");
        Texture texture;
        if (pathIt == tex.end()) {
            texture = Texture::defaultTexture(defaultTexture);
        } else {
            auto fspath = rootPath / std::filesystem::u8path(static_cast<std::string>(tex["texture"]));
            auto path = fspath.string();
            auto textIt = tex.find("encoding");
            if (textIt != tex.end()) {
                if (*textIt == "linear") {
                    defaultEncoding = Texture::Encoding::LINEAR;
                } else if (*textIt == "srgb") {
                    defaultEncoding = Texture::Encoding::SRGB;
                }
            }
            texture = Application::loadTexture(path.c_str(), defaultEncoding);
        }
        auto intensity = getOrDefault(tex, intensityKey, T{1.0f});
        return std::make_pair(texture, intensity);
    };

    Material *readMaterial(nlohmann::json &j) const {
        auto &type = j["type"];
        if (type == "standard") {
            auto stdMtl = new StandardMaterial;
            auto albedo = loadTexture<glm::vec4>("albedo", "color", j,
                Texture::Encoding::LINEAR, Texture::DefaultTexture::WHITE);
            stdMtl->albedoMap = albedo.first;
            stdMtl->color = albedo.second;
            auto metallic = loadTexture<float>("metallic", "intensity", j,
                Texture::Encoding::LINEAR, Texture::DefaultTexture::WHITE);
            stdMtl->metallicMap = metallic.first;
            stdMtl->metallicIntensity = metallic.second;
            auto roughness = loadTexture<float>("roughness", "intensity", j,
                Texture::Encoding::LINEAR, Texture::DefaultTexture::WHITE);
            stdMtl->roughnessMap = roughness.first;
            stdMtl->roughnessIntensity = roughness.second;
            auto ao = loadTexture<float>("ao", "intensity", j,
                Texture::Encoding::LINEAR, Texture::DefaultTexture::WHITE);
            stdMtl->aoMap = ao.first;
            stdMtl->aoIntensity = ao.second;
            auto normal = loadTexture<float>("normal", "intensity", j,
                Texture::Encoding::LINEAR, Texture::DefaultTexture::DEFAULT_NORMAL);
            stdMtl->normalMap = normal.first;
            stdMtl->emission = getOrDefault(j, "emission", glm::vec3(0.0f));
            stdMtl->ior = getOrDefault(j, "ior", 1.0f);
            stdMtl->specTrans = getOrDefault(j, "specTrans", 0.0f);
            stdMtl->transparent = getOrDefault(j, "transparent", false);
            return stdMtl;
        } else if (type == "phong") {
            auto phongMtl = new PhongMaterial;
            auto albedo = loadTexture<glm::vec4>("albedo", "color", j,
                Texture::Encoding::LINEAR, Texture::DefaultTexture::WHITE);
            phongMtl->diffuse = albedo.first;
            phongMtl->color = albedo.second;
            phongMtl->shininess = getOrDefault(j, "shiniess", 1.0f);
            phongMtl->emission = getOrDefault(j, "emission", glm::vec3(0.0f));
            return phongMtl;
        }
        return nullptr;
    }

    Object3D *toPlaceHolder(nlohmann::json &j) const {
        auto &position = j["position"];
        auto &rotation = j["rotation"];

        auto *obj = new Object3D;
        obj->setPosition(glm::vec3{position[0], position[1], position[2]});
        obj->applyRotation(glm::quat{rotation[0], rotation[1], rotation[2], rotation[3]});
        if (j.contains("children")) {
            for (auto &child: j["children"]) {
                obj->addChild(toObject(child));
            }
        }
        return obj;
    };

    cg::Object3D *load(nlohmann::json &j) {
        auto *root = new Object3D;
        try {
            for (auto &mtlObj: j["materials"]) {
                auto *mtl = readMaterial(mtlObj);
                if (mtl) {
                    saveMaterial(mtlObj["id"], mtl);
                }
            }
            for (auto &obj: j["objects"]) {
                auto *o = toObject(obj);
                if (o) root->addChild(o);
            }
        } catch (std::exception &ex) {
            delete root;
            return nullptr;
        }
        return root;
    }
};

cg::Object3D *cg::loadJsonScene(const char *path) {
    try {
        auto fspath = std::filesystem::u8path(path);
        std::ifstream i(fspath);
        nlohmann::json j;
        i >> j;
        return ObjLoader{fspath.parent_path()}.load(j);
    } catch (std::exception &ex) {
        return nullptr;
    }
}
