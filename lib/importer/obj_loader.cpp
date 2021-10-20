#include <obj_loader.h>
#include <cg_common.h>
#include <object3d.h>
#include <mesh.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <vector>
#include <regex>
#include <optional>
#include <functional>

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

    Object3D *root = new Object3D;
    const auto convert_mesh = [&](unsigned int meshIdx) -> Mesh * {
        return nullptr;
    };
    const std::function<void(aiNode *, Object3D *)> dfs = [&](aiNode *node, Object3D *parent) {
        Object3D *nodeObj = new Object3D;
        nodeObj->setLocalMatrix(convertMatrix4(node->mTransformation));
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
            dfs(node->mChildren[i], parent);
        }
    };
}
