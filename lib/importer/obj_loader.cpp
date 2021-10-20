#include <obj_loader.h>
#include <cg_common.h>
#include <object3d.h>
#include <mesh.h>
#include <geometry.h>
#include <material.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <vector>
#include <regex>
#include <optional>
#include <functional>
#include <map>

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

    std::map<unsigned int, std::shared_ptr<Geometry>> geometries;
    std::map<unsigned int, std::shared_ptr<Material>> mats;
    auto *root = new Object3D;
    std::vector<float> vertices;
    const auto convert_mesh = [&](unsigned int meshIdx) -> Mesh * {
        aiMesh *mesh = scene->mMeshes[meshIdx];
        // geometry
        std::shared_ptr<Geometry> geo;
        const auto it_geo = geometries.find(meshIdx);
        if (it_geo != geometries.end()) {
            geo = it_geo->second;
        } else {
            geo = std::make_shared<Geometry>();
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
                geo->addAttribute("uv", vertices.data(), vertices.size(), 2);
            }
        }

        // material
        auto it_mat = mats.find(mesh->mMaterialIndex);
        std::shared_ptr<Material> mat;
        if (it_mat != mats.end()) {
            mat = it_mat->second;
        } else {
            mat = std::make_shared<PhongMaterial>();
            aiMaterial *ai_mat = scene->mMaterials[mesh->mMaterialIndex];
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
