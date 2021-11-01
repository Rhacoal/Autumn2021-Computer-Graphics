#ifndef ASSIGNMENT_RT_SCENE_H
#define ASSIGNMENT_RT_SCENE_H

#include <cg_common.h>
#include <cg_fwd.h>
#include <texture.h>
#include "lib/shaders/rt_structure.h"
#include "shader.h"

#include <cl.hpp>

#include <optional>

namespace cg {
struct BVH {
    std::vector<BVHNode> nodes;

    void buildFromTriangles(std::vector<Triangle> &triangles);
};

class RayTracingScene {
    BVH bvh;

    // BVH buffer
    size_t bvhNodeCount;
    cl::Buffer bvhBuffer;

    // primitives buffer
    size_t triangleCount;
    cl::Buffer triangleBuffer;
public:
    void setFromScene(Scene &scene);
};

class RayTracingRenderer {
    typedef struct ShaderParams {
        Shader shader;
        GLuint vao{};
        GLuint vbo{};

        ShaderParams();

        void use() const;

        ~ShaderParams();
    } ShaderParams;
    inline static std::optional<ShaderParams> trivialShader;

    cl::Platform platform;
    cl::Device device;
    cl::Context context;
    cl::CommandQueue commandQueue;
    cg::Texture texture;

    bool clInited = false;

    bool initCL();

    bool programInited = false;

    cl::Program program;
    cl::Kernel testKernel;
    cl::Kernel rayGenerationKernel;
    cl::Kernel renderKernel;

    int _width{}, _height{};
    cl::Buffer rayBuffer;
    cl::Buffer testBuffer;

    std::vector<float> frameBuffer;

    size_t frameBufferSize() const {
        return frameBuffer.size() * sizeof(float);
    }

public:
    bool init(int width, int height);

    void render(RayTracingScene &scene, Camera &camera);
};
}

#endif //ASSIGNMENT_RT_SCENE_H
