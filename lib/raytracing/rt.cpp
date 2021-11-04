#include <rt.h>
#include <scene.h>
#include <camera.h>
#include <geometry.h>
#include <material.h>
#include <mesh.h>
#include "lib/shaders/rt_definition.h"

#include <cl.hpp>
#include <tinyfiledialogs.h>

#include <vector>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <functional>
#include <random>
#include <thread>

void cg::RayTracingScene::setFromScene(cg::Scene &scene) {
    bufferNeedUpdate = true;
    materials.clear();
    triangles.clear();
    lights.clear();
    std::map<Material *, uint32_t> mtlMap;
    scene.traverse([&](Object3D &object) {
        Mesh *mesh = object.isMesh();
        if (!mesh || mesh->isBackground()) return;
        MeshGeometry *geometry = mesh->geometry();
        Material *mtl = mesh->material();
        auto bufInfo = [](
            const std::optional<const MeshGeometry::Attribute *> &buf) -> std::optional<std::pair<const MeshGeometry::buffer_t, unsigned int>> {
            if (buf.has_value()) {
                return std::make_optional(std::make_pair(buf.value()->buf, buf.value()->itemSize));
            }
            return {};
        };
        uint mtlIndex;
        auto mtlIt = mtlMap.find(mtl);
        if (mtlIt != mtlMap.end()) {
            mtlIndex = mtlIt->second;
        } else {
            auto rtMtl = RayTracingMaterial{
                .albedo = toFloat4(mtl->color),
                .emission = toFloat4(mtl->emmision),
                .metallic = 0.5f,
                .roughness = 0.5f,
                .specTrans = 0.0f,
                .ior = 1.35f,
            };
            mtlIndex = materials.size();
            mtlMap[mtl] = mtlIndex;
            materials.emplace_back(rtMtl);
        }
        auto position = bufInfo(geometry->getAttribute("position"));
        if (!position.has_value()) return;
        auto texcoord = bufInfo(geometry->getAttribute("texcoord"));
        auto normal = bufInfo(geometry->getAttribute("normal"));
        auto modelMatrix = object.modelMatrix();
        const auto transform = [&](auto &&buf, uint v0, uint size) -> float4 {
            return toFloat4(modelMatrix * glm::vec4{
                buf[v0 * size + 0], buf[v0 * size + 1], buf[v0 * size + 2], 1.0
            });
        };
        const auto addTriangle = [&](unsigned int v0, unsigned int v1, unsigned int v2) {
            Triangle triangle{
                .mtlIndex = mtlIndex,
            };
            // position
            auto[buf, size] = position.value();
            triangle.v0.position = transform(buf, v0, size);
            triangle.v1.position = transform(buf, v1, size);
            triangle.v2.position = transform(buf, v2, size);
            // texcoord
            if (texcoord.has_value()) {
                auto[buf, size] = texcoord.value();
                triangle.v0.texcoord = float2{buf[v0 * size + 0], buf[v0 * size + 1]};
                triangle.v1.texcoord = float2{buf[v1 * size + 0], buf[v1 * size + 1]};
                triangle.v2.texcoord = float2{buf[v2 * size + 0], buf[v2 * size + 1]};
            }
            // normal
            if (normal.has_value()) {
                auto[buf, size] = normal.value();
                triangle.v0.normal = float3{buf[v0 * size + 0], buf[v0 * size + 1], buf[v0 * size + 2]};
                triangle.v1.normal = float3{buf[v1 * size + 0], buf[v1 * size + 1], buf[v1 * size + 2]};
                triangle.v2.normal = float3{buf[v2 * size + 0], buf[v2 * size + 1], buf[v2 * size + 2]};
            }
            triangles.emplace_back(triangle);
        };
        if (geometry->hasIndices()) {
            const auto indices = geometry->getIndices().value();
            for (unsigned int i = 0; i < indices.size() - 2; i += 3) {
                addTriangle(indices[i], indices[i + 1], indices[i + 2]);
            }
        } else {
            const size_t vertices = position.value().first.size();
            for (size_t i = 0; i < vertices - 2; i += 3) {
                addTriangle(i, i + 1, i + 2);
            }
        }
    });
    // prevent empty buffers
    if (triangles.empty()) {
        triangles.emplace_back();
    }
    if (materials.empty()) {
        materials.emplace_back();
    }
    bvh.buildFromTriangles(triangles);
}

struct CPUDispatcher {
    int cores = 1;

    template<typename Func, typename...Args>
    void dispatch(uint size, Func &&func, Args &&...args) {
        if (cores == 1) {
            for (uint i = 0; i < size; ++i) {
                set_global_id(0, i);
                func(std::forward<Args &&>(args)...);
            }
        } else {
            std::vector<std::thread> threads;
            for (int i = 0; i < cores; ++i) {
                int coreId = i;
                threads.emplace_back([&]() {
                    for (uint t = coreId; t < size; t += cores) {
                        set_global_id(0, t);
                        func(std::forward<Args &&>(args)...);
                    }
                });
            }
            for (auto &thread : threads) {
                thread.join();
            }
        }
    }
};

void cg::RayTracingRenderer::renderCPU(cg::RayTracingScene &scene, cg::Camera &camera) {
    // no need to init cl first
    // TODO init texture buffer
//    auto textureMemBuffer = nullptr;
    auto triangleMemBuffer = scene.triangles.data();
    auto materialMemBuffer = scene.materials.data();
    auto bvhMemBuffer = scene.bvh.nodes.data();
    rayMemBuffer.resize(_width * _height);
    accumulateFrameBuffer.resize(_width * _height * 4);
    if (camera.up() != up || camera.lookDir() != dir || camera.position() != pos) {
        up = camera.up();
        dir = camera.lookDir();
        pos = camera.position();
        samples = 0;
        std::fill(accumulateFrameBuffer.begin(), accumulateFrameBuffer.end(), 0.0f);
    }
    samples += 1;
    CPUDispatcher dispatcher{.cores = 1};
    // __global Ray *output,
    // uint width, uint height, ulong globalSeed,
    // float3 cameraPosition, float3 cameraDir, float3 cameraUp, float fov, float near
    PerspectiveCamera *perspectiveCamera = camera.isPerspectiveCamera();

    dispatcher.dispatch(_width * _height, raygeneration_kernel,
        rayMemBuffer.data(), _width, _height, std::uniform_int_distribution<ulong>()(engine),
        toFloat3(camera.position()), toFloat3(camera.lookDir()), toFloat3(camera.up()),
        perspectiveCamera->fov() / 180.f * math::pi<float>(), perspectiveCamera->near());
    // __global float4 *output, uint width, uint height,
    // __global BVHNode *bvh, __global Triangle *triangles, __global RayTracingMaterial *materials,
    // __global Ray *rays, uint bounces,
    // ulong globalSeed, uint spp
    dispatcher.dispatch(_width * _height, render_kernel,
        reinterpret_cast<float4 *>(accumulateFrameBuffer.data()), _width, _height,
        bvhMemBuffer, triangleMemBuffer, materialMemBuffer,
        rayMemBuffer.data(), 2,
        std::uniform_int_distribution<ulong>()(engine), 1
    );
    CPUDispatcher{.cores = 1}.dispatch(_width * _height * 4, [&]() {
        uint id = get_global_id(0);
        frameBuffer[id] = accumulateFrameBuffer[id] / samples;
    });
    drawFrameBuffer();
}

void cg::RayTracingRenderer::render(RayTracingScene &scene, Camera &camera) {
    // update buffers (if needed)
    int err;
    if (scene.bufferNeedUpdate || sceneBufferNeedUpdate) {
        // TODO actually fill in these buffers
        // textureBuffer = cl::Buffer(context, CL_MEM_READ_ONLY, (size_t) 24, nullptr, &err);
        triangleBuffer = cl::Buffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
            scene.triangles.size() * sizeof(Triangle), scene.triangles.data(), &err);
        materialBuffer = cl::Buffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
            scene.materials.size() * sizeof(RayTracingScene), scene.materials.data(), &err);
        // scene.materials.size() * sizeof(RayTracingMaterial), scene.materials.data(), &err);
        bvhBuffer = cl::Buffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
            scene.bvh.nodes.size() * sizeof(BVHNode), scene.bvh.nodes.data(), &err);

        // __global Ray *output,
        rayGenerationKernel.setArg(0, rayBuffer());
        // uint width, uint height,
        rayGenerationKernel.setArg(1, _width);
        rayGenerationKernel.setArg(2, _height);
        // ulong globalSeed,
        rayGenerationKernel.setArg(3, std::uniform_int_distribution<ulong>{}(engine));

        // __global float4 *output, uint width, uint height,
        renderKernel.setArg(0, accumulateBuffer());
        renderKernel.setArg(1, _width);
        renderKernel.setArg(2, _height);
        // __global BVHNode *bvh, __global Triangle *triangles, __global RayTracingMaterial *materials,
        renderKernel.setArg(3, bvhBuffer());
        renderKernel.setArg(4, triangleBuffer());
        renderKernel.setArg(5, materialBuffer());
        // __global Ray *rays, uint bounces,
        renderKernel.setArg(6, rayBuffer());
        renderKernel.setArg(7, bounces);
        // ulong globalSeed, uint spp
        renderKernel.setArg(8, std::uniform_int_distribution<ulong>{}(engine));
        renderKernel.setArg(9, uint(1));

        // __global float4 *output, uint width, uint height
        clearKernel.setArg(0, accumulateBuffer());
        clearKernel.setArg(1, _width);
        clearKernel.setArg(2, _width);

        // __global float4 *input, uint width, uint height, uint spp, __global float4 *output
        accumulateKernel.setArg(0, accumulateBuffer());
        accumulateKernel.setArg(1, _width);
        accumulateKernel.setArg(2, _height);
        accumulateKernel.setArg(3, uint(1));
        accumulateKernel.setArg(4, outputBuffer());

        scene.bufferNeedUpdate = false;
        sceneBufferNeedUpdate = false;

        printf("Scene inited for RT");
    }
    // update camera
    auto perspective = camera.isPerspectiveCamera();
    if (!perspective) {
        throw std::runtime_error("Only perspective camera can be used for path tracing.");
    }
    // resample
    bool needClear = false;
    if (camera.up() != up || camera.lookDir() != dir || camera.position() != pos) {
        needClear = true;
        up = camera.up();
        dir = camera.lookDir();
        pos = camera.position();
        samples = 0;
    }
    samples += 1;
    // float3 cameraPosition, float3 cameraDir, float3 cameraUp,
    rayGenerationKernel.setArg(4, toFloat3(pos));
    rayGenerationKernel.setArg(5, toFloat3(dir));
    rayGenerationKernel.setArg(6, toFloat3(up));
    // float fov, float near
    rayGenerationKernel.setArg(7, (perspective->fov() / 180.f * math::pi<float>()));
    rayGenerationKernel.setArg(8, perspective->near());

    // random seed
    rayGenerationKernel.setArg(3, std::uniform_int_distribution<ulong>{}(engine));
    renderKernel.setArg(8, std::uniform_int_distribution<ulong>{}(engine));

    // spp
    accumulateKernel.setArg(3, static_cast<uint>(samples));

    // ray tracing
    cl::Event rayGen;
    err = commandQueue.enqueueNDRangeKernel(
        rayGenerationKernel, cl::NullRange, _width * _height, cl::NullRange, nullptr, &rayGen
    );
    std::vector<cl::Event> preRenderEvents = {rayGen};
    if (needClear) {
        cl::Event ev;
        err = commandQueue.enqueueNDRangeKernel(clearKernel, cl::NullRange, _width * _height, cl::NullRange, nullptr,
            &ev);
        preRenderEvents.emplace_back(ev);
    }
    std::vector<cl::Event> raytracingEvent(1);
    err = commandQueue.enqueueNDRangeKernel(
        renderKernel, cl::NullRange, _width * _height, cl::NullRange, &preRenderEvents, raytracingEvent.data()
    );
    err = raytracingEvent[0].wait();
    std::vector<cl::Event> accumulateEvent(1);
    err = commandQueue.enqueueNDRangeKernel(
        accumulateKernel, cl::NullRange, _width * _height, cl::NullRange, &raytracingEvent, accumulateEvent.data()
    );
    err = commandQueue.enqueueReadBuffer(
        outputBuffer, CL_TRUE, 0, frameBufferSize(), frameBuffer.data(), &accumulateEvent, nullptr
    );
    commandQueue.finish();
    // draw to screen
    drawFrameBuffer();
}

void cg::RayTracingRenderer::drawFrameBuffer() {
    texture.setData(_width, _height, GL_RGB, GL_RGBA, GL_FLOAT, frameBuffer.data());
    if (!trivialShader.has_value()) {
        trivialShader.emplace();
    }
    check_err(trivialShader->use());
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture.tex());
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

bool cg::RayTracingRenderer::initCL() {
    if (clInited) return true;
    // Get all available OpenCL platforms (e.g. AMD OpenCL, Nvidia CUDA, Intel OpenCL)
    std::vector<cl::Platform> platforms;
    cl::Platform::get(&platforms);
    const char *endl = "\\r\\n";
    int platform_index = -1;
    {
        std::stringstream text;
        text << "Available OpenCL platforms:" << endl;
        for (int i = 0; i < platforms.size(); ++i) {
            auto name = platforms[i].getInfo<CL_PLATFORM_NAME>();
            text << "    " << i + 1 << ": " << name.c_str() << endl;
        }
        text << "Enter a number: (1 ~ " << platforms.size() << ')';

        platform_index = 0;
        // Pick one platform
        while (platform_index == -1) {
            auto text_str = text.str();
            char *ret = tinyfd_inputBox("Select a OpenCL platform", text_str.c_str(), "");
            if (!ret) {
                return false;
            }
            char *endptr = nullptr;
            long int idx = std::strtol(ret, &endptr, 10);
            if (endptr == ret + strlen(ret) && idx >= 1 && idx <= platforms.size()) {
                platform_index = static_cast<int>(idx - 1);
            }
        }
    }
    platform = platforms[platform_index];
    std::vector<cl::Device> devices;
    platform.getDevices(CL_DEVICE_TYPE_ALL, &devices);
    int device_index = -1;
    {
        std::stringstream text;
        auto platform_name = platform.getInfo<CL_PLATFORM_NAME>();
        text << "Available OpenCL devices on this platform: "
             << platform_name.c_str() << endl << endl;
        for (int i = 0; i < devices.size(); i++) {
            auto name = devices[i].getInfo<CL_DEVICE_NAME>();
            text << "\t" << i + 1 << ": "
                 << name.c_str() << endl;
            text << "\t\tMax compute units: "
                 << devices[i].getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>() << endl;
            text << "\t\tMax work group size: "
                 << devices[i].getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>() << endl << endl;
        }
        text << "Enter a number (1 ~ " << devices.size() << ')';

//        device_index = 0;
        while (device_index == -1) {
            auto text_str = text.str();
            char *ret = tinyfd_inputBox("Select a OpenCL device", text_str.c_str(), "");
            if (!ret) {
                return false;
            }
            char *endptr = nullptr;
            long int idx = std::strtol(ret, &endptr, 10);
            if (endptr == ret + strlen(ret) && idx >= 1 && idx <= devices.size()) {
                device_index = static_cast<int>(idx - 1);
            }
        }
    }
    device = devices[device_index];
    context = cl::Context(device);
    commandQueue = cl::CommandQueue(context, device);
    clInited = true;
    return true;
}

bool cg::RayTracingRenderer::init(int width, int height) {
    initCL();
    if (!programInited) {
        if (!reloadShader()) {
            return false;
        }
        programInited = true;
    }
    if (width != _width || height != _height) {
        _width = width;
        _height = height;
        // prepare buffers
        frameBuffer.resize(_width * _height * 4);
        std::fill(frameBuffer.begin(), frameBuffer.end(), 0.5f);

        int err;
        rayBuffer = cl::Buffer(context, CL_MEM_READ_WRITE, width * height * sizeof(Ray), nullptr, &err);
        outputBuffer = cl::Buffer(context, CL_MEM_READ_WRITE, width * height * sizeof(float4), nullptr, &err);
        accumulateBuffer = cl::Buffer(context, CL_MEM_READ_WRITE, width * height * sizeof(float4), nullptr, &err);
        sceneBufferNeedUpdate = true;
    }
    return true;
}

bool cg::RayTracingRenderer::reloadShader() {
    std::string source = readFile("lib/shaders/raytracing.cpp");
    commandQueue.finish();
    sceneBufferNeedUpdate = true;
    program = cl::Program(context, source);
    cl_int result = program.build({device});
    if (result) {
        fprintf(stderr, "error during compilation (%d):\n", result);
    }
    if (result == CL_BUILD_PROGRAM_FAILURE) {
        auto buildLog = program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device);
        puts(buildLog.c_str());
        return false;
    }
    rayGenerationKernel = cl::Kernel(program, "raygeneration_kernel");
    renderKernel = cl::Kernel(program, "render_kernel");
    accumulateKernel = cl::Kernel(program, "accumulate_kernel");
    clearKernel = cl::Kernel(program, "clear_kernel");
    return true;
}

void cg::RayTracingRenderer::setBounces(uint newBounces) {
    this->bounces = std::clamp(newBounces, uint(1), uint(15));
}

int cg::RayTracingRenderer::sampleCount() const {
    return samples;
}

template<typename ...Args>
static uint push(std::vector<BVHNode> &nodes, Args &&...args) {
    uint ret = static_cast<uint>(nodes.size());
    nodes.emplace_back(std::forward<Args &&>(args)...);
    return ret;
};

uint recur(std::vector<BVHNode> &nodes, std::vector<Triangle>::iterator first, std::vector<Triangle>::iterator last,
           uint baseIndex) {
    auto length = last - first;
    if (length == 1) {
        return push(nodes, BVHNode{first->bounds(), baseIndex, 1}); // leaf
    }
    if (length == 2) {
        auto bounds = first->bounds() + std::next(first)->bounds();
        unsigned short dim = bounds.maxExtent();
        auto ret = push(nodes, BVHNode{bounds, 0, 0, dim});
        if (first->bounds().centroid().s[dim] < (std::next(first))->bounds().centroid().s[dim]) {
            // left
            recur(nodes, first, std::next(first), baseIndex);
            // right
            nodes[ret].offset = recur(nodes, std::next(first), last, baseIndex + 1);
        } else {
            // right
            recur(nodes, std::next(first), last, baseIndex + 1);
            // left
            nodes[ret].offset = recur(nodes, first, std::next(first), baseIndex);
        }
        return ret;
    }
    auto bounds = first->bounds();
    auto centroidBounds = Bounds3(bounds.centroid());
    for (auto i = std::next(first); i != last; ++i) {
        Bounds3 iBound = i->bounds();
        bounds += iBound;
        centroidBounds += iBound.centroid();
    }
    auto maxExtend = bounds.maxExtent();
    std::sort(first, last, [maxExtend](const Triangle &a, const Triangle &b) -> bool {
        return a.bounds().centroid().s[maxExtend] < b.bounds().centroid().s[maxExtend];
    });

    auto leftSize = length / 2;
    auto ret = push(nodes, BVHNode{bounds, 0, 0, maxExtend});
    recur(nodes, first, first + leftSize, baseIndex);
    nodes[ret].offset = recur(nodes, first + leftSize, last, baseIndex + leftSize);
    return ret;
};

void cg::BVH::buildFromTriangles(std::vector<Triangle> &triangles) {
    nodes.clear();
    recur(nodes, triangles.begin(), triangles.end(), 0u);
}

const char *trivialShaderVert = R"(
#version 330 core

layout (location = 0) in vec2 inTexCoord;
out vec2 texCoord;

uniform sampler2D screenTexture;

void main() {
    texCoord = inTexCoord;
    gl_Position = vec4(inTexCoord * 2.0 - 1.0, 0.0, 1.0);
}
)";
const char *trivialShaderFrag = R"(
#version 330 core

uniform sampler2D screenTexture;
in vec2 texCoord;
out vec4 fragColor;

void main() {
//    fragColor = vec4(texCoord, 0.0, 1.0);
    fragColor = texture(screenTexture, texCoord);
}
)";

static constexpr float vertices[] = {0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 1.0, 0.0};

cg::RayTracingRenderer::ShaderParams::ShaderParams() : shader(trivialShaderVert, trivialShaderFrag) {
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, nullptr);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    shader.use();
    shader.setUniform1i("screenTexture", 0);
}

cg::RayTracingRenderer::ShaderParams::~ShaderParams() {
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
}

void cg::RayTracingRenderer::ShaderParams::use() const {
    shader.use();
    glBindVertexArray(vao);
}
