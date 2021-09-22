#ifndef ASSIGNMENT_RT_SCENE_H
#define ASSIGNMENT_RT_SCENE_H

#include <cg_common.h>
#include <cg_fwd.h>
#include <texture.h>
#include <shader.h>
#include "lib/shaders/rt_structure.h"

#ifndef NO_CL

#include <cl.hpp>

#endif

#include <optional>
#include <random>

namespace cg {
struct BVH {
    std::vector<BVHNode> nodes;

    void buildFromTriangles(std::vector<Triangle> &triangles);
};

struct RayTracingScene {
    bool bufferNeedUpdate = true;

    BVH bvh;
    std::vector<RayTracingTextureRange> textures;
    std::vector<float> textureData;
    std::vector<Triangle> triangles;
    std::vector<RayTracingMaterial> materials;
    std::vector<uint> lights;

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

#ifndef NO_CL
    cl::Platform platform;
    cl::Device device;
    cl::Context context;
    cl::CommandQueue commandQueue;

    bool clInited = false;
    bool programInited = false;

    bool initCLDevice();

    cl::Program program;
    cl::Kernel testKernel;
    cl::Kernel rayGenerationKernel;
    cl::Kernel renderKernel;
    cl::Kernel accumulateKernel;
    cl::Kernel clearKernel;

    cl::Buffer rayBuffer;
    cl::Buffer seedBuffer;
    cl::Buffer accumulateBuffer;
    cl::Buffer outputBuffer;

    // scene related buffers
    cl::Buffer triangleBuffer;
    cl::Buffer materialBuffer;
    cl::Buffer textureRangeBuffer;
    cl::Buffer textureDataBuffer;
    cl::Buffer bvhBuffer;
    cl::Buffer lightBuffer;
#endif

    bool sceneBufferNeedUpdate = true;

    int _width{}, _height{};

    cg::Texture texture;
    std::vector<float> accumulateFrameBuffer;
    std::vector<float> frameBuffer;

    // camera and resampling
    glm::vec3 up, dir, pos;
    uint samples = 0;
    uint spp = 1;

    // cpu related buffers
    std::vector<float3> rayMemBuffer;
    std::vector<ulong> seedMemBuffer;

    // random generator
    std::random_device r{};
    std::default_random_engine engine{r()};

    // raytracing config
    uint bounces = 10;

    size_t frameBufferSize() const {
        return frameBuffer.size() * sizeof(float);
    }

    void initFrameBuffer(int width, int height);

    void drawFrameBuffer();

public:
    bool initCL(int width, int height);

    void render(RayTracingScene &scene, Camera &camera);

    bool reloadShader();

    bool initCPU(int width, int height);

    void renderCPU(RayTracingScene &scene, Camera &camera);

    void setBounces(uint newBounces);

    const float *frameBufferData() const noexcept;

    int sampleCount() const noexcept;

    int width() const noexcept;

    int height() const noexcept;
};
}

#endif //ASSIGNMENT_RT_SCENE_H
