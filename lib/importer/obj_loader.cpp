#include <obj_loader.h>
#include <cg_common.h>
#include <object3d.h>
#include <mesh.h>
#include <geometry.h>
#include <material.h>
#include <application.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
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

glm::mat4 convertMatrix4(const aiMatrix4x4 &mat4) {
    return glm::mat4{
        {mat4.a1, mat4.a2, mat4.a3, mat4.a4},
        {mat4.b1, mat4.b2, mat4.b3, mat4.b4},
        {mat4.c1, mat4.c2, mat4.c3, mat4.c4},
        {mat4.d1, mat4.d2, mat4.d3, mat4.d4},
    };
}

cg::Object3D *cg::loadObj(const char *path) {
    // TODO finish .obj file reader
    Assimp::Importer importer;

    const aiScene *scene = importer.ReadFile(path,
        aiProcess_CalcTangentSpace |
        aiProcess_Triangulate |
        aiProcess_JoinIdenticalVertices |
        aiProcess_SortByPType);
    if (!scene) {
        fprintf(stderr, "failed to load obj: %s\n%s\n", path, importer.GetErrorString());
        return nullptr;
    }

    std::map<unsigned int, std::shared_ptr<MeshGeometry>> geometries;
    std::map<unsigned int, std::shared_ptr<Material>> mats;
    auto *root = new Object3D;
    std::vector<float> vertices;
    const auto convert_mesh = [&](unsigned int meshIdx) -> Mesh * {
        aiMesh *mesh = scene->mMeshes[meshIdx];
        // geometry
        std::shared_ptr<MeshGeometry> geo;
        const auto it_geo = geometries.find(meshIdx);
        if (it_geo != geometries.end()) {
            geo = it_geo->second;
        } else {
            geo = std::make_shared<MeshGeometry>();
            if (!mesh->HasPositions() || !mesh->HasFaces()) {
                return nullptr;
            }
            // positions
            vertices.clear();
            for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
                const aiVector3D &vec3 = mesh->mVertices[i];
                vertices.insert(vertices.end(), {vec3.x, vec3.y, vec3.z});
            }
            geo->addAttribute("position", vertices.data(), vertices.size(), 3);
            vertices.clear();
            // indices
            std::vector<unsigned int> indices;
            for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
                const aiFace &face = mesh->mFaces[i];
                // aiProcess_Triangulate is applied, so we may assumes that face.mNumIndices == 3
                indices.insert(indices.end(), {face.mIndices[0], face.mIndices[1], face.mIndices[2]});
            }
            geo->addIndices(indices.data(), indices.size());
            // normals
            if (mesh->HasNormals()) {
                vertices.clear();
                for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
                    const aiVector3D &vec3 = mesh->mNormals[i];
                    vertices.insert(vertices.end(), {vec3.x, vec3.y, vec3.z});
                }
                geo->addAttribute("normal", vertices.data(), vertices.size(), 3);
            }
            // vertex uv
            if (mesh->HasTextureCoords(0)) {
                vertices.clear();
                for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
                    const aiVector3D &vec3 = mesh->mTextureCoords[0][i];
                    // assume 2D uv
                    vertices.insert(vertices.end(), {vec3.x, vec3.y});
                }
                geo->addAttribute("texcoord", vertices.data(), vertices.size(), 2);
            }
        }

        // material
        auto it_mat = mats.find(mesh->mMaterialIndex);
        std::shared_ptr<Material> mat;
        if (it_mat != mats.end()) {
            mat = it_mat->second;
        } else {
            auto phong = std::make_shared<PhongMaterial>();
            aiMaterial *ai_mat = scene->mMaterials[mesh->mMaterialIndex];
            // diffuse texture
            aiString diffuse;
            ai_mat->Get(AI_MATKEY_TEXTURE_DIFFUSE(0), diffuse);
            if (diffuse.length) {
                phong->diffuse = Application::loadTexture(diffuse.C_Str(), Texture::Encoding::LINEAR);
            }
            mat = std::move(phong);
        }

        // mesh
        return new Mesh(std::move(mat), std::move(geo));
    };
    const std::function<void(aiNode *, Object3D *)> convert_scene = [&](aiNode *node, Object3D *parent) {
        auto *node_obj = new Object3D;
        node_obj->setLocalMatrix(convertMatrix4(node->mTransformation));
        if (node->mNumMeshes) {
            for (unsigned int i = 0; i < node->mNumMeshes; ++i) {
                Mesh *mesh = convert_mesh(node->mMeshes[i]);
                if (mesh) {
                    parent->addChild(mesh);
                }
            }
        }

        // continue for all child nodes
        for (unsigned int i = 0; i < node->mNumChildren; ++i) {
            convert_scene(node->mChildren[i], node_obj);
        }
    };
    convert_scene(scene->mRootNode, root);
    return root;
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
