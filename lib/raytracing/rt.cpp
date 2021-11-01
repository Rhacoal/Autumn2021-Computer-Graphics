#include <rt.h>
#include <scene.h>
#include <camera.h>
#include <geometry.h>
#include <material.h>
#include <mesh.h>

#include <cl.hpp>
#include <tinyfiledialogs.h>

#include <vector>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <functional>

void cg::RayTracingScene::setFromScene(cg::Scene &scene) {
    std::vector<RayTracingMaterial> rtMaterial;
    std::map<Material *, uint32_t> mtlMap;
    std::vector<Triangle> triangles;
    scene.traverse([&](Object3D &object) {
        Mesh *mesh = object.isMesh();
        if (!mesh) return;
        Geometry *geometry = mesh->geometry();
        Material *mtl = mesh->material();
        auto bufInfo = [](
            const std::optional<const Geometry::Attribute *> &buf) -> std::optional<std::pair<const Geometry::buffer_t, unsigned int>> {
            if (buf.has_value()) {
                return std::make_optional(std::make_pair(buf.value()->buf, buf.value()->itemSize));
            }
            return {};
        };
        uint32_t mtlIndex;
        auto mtlIt = mtlMap.find(mtl);
        if (mtlIt != mtlMap.end()) {
            mtlIndex = mtlIt->second;
        } else {
            auto mtl = RayTracingMaterial {
                .color = {0.5f, 0.5f, 1.0f, 1.0f},
                .ior = 1.35f,
            };
        }
        auto position = bufInfo(geometry->getAttribute("position"));
        if (!position.has_value()) return;
        auto texcoord = bufInfo(geometry->getAttribute("texcoord"));
        auto normal = bufInfo(geometry->getAttribute("normal"));
        const auto addTriangle = [&](unsigned int v0, unsigned int v1, unsigned int v2) {
            Triangle triangle{};
            if (true) {
                auto[buf, size] = position.value();
                triangle.v0.position = float3{buf[v0 * size + 0], buf[v0 * size + 1], buf[v0 * size + 2]};
                triangle.v1.position = float3{buf[v1 * size + 0], buf[v1 * size + 1], buf[v1 * size + 2]};
                triangle.v2.position = float3{buf[v2 * size + 0], buf[v2 * size + 1], buf[v2 * size + 2]};
            }
            if (texcoord.has_value()) {
                auto[buf, size] = texcoord.value();
                triangle.v0.texcoord = float3{buf[v0 * size + 0], buf[v0 * size + 1]};
                triangle.v1.texcoord = float3{buf[v1 * size + 0], buf[v1 * size + 1]};
                triangle.v2.texcoord = float3{buf[v2 * size + 0], buf[v2 * size + 1]};
            }
            if (normal.has_value()) {
                auto[buf, size] = normal.value();
                triangle.v0.normal = float3{buf[v0 * size + 0], buf[v0 * size + 1], buf[v0 * size + 2]};
                triangle.v1.normal = float3{buf[v1 * size + 0], buf[v1 * size + 1], buf[v1 * size + 2]};
                triangle.v2.normal = float3{buf[v2 * size + 0], buf[v2 * size + 1], buf[v2 * size + 2]};
            }
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
}

void cg::RayTracingRenderer::render(RayTracingScene &scene, Camera &camera) {
    // ray tracing
    std::vector<cl::Event> evs(1);
    commandQueue.enqueueNDRangeKernel(testKernel, cl::NullRange, _width * _height, cl::NullRange, nullptr,
        &evs[0]);
    commandQueue.enqueueReadBuffer(testBuffer, CL_TRUE, 0, frameBufferSize(), frameBuffer.data(), &evs, nullptr);
//    commandQueue.enqueueReadBuffer(testBuffer, CL_TRUE, 0, 100 * 3, test, &evs, nullptr);
    commandQueue.finish();
    // draw to screen
//    std::fill(frameBuffer.begin(), frameBuffer.end(), 0.5f);
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
    int platform_index = -1;
    {
        std::stringstream text;
        text << "Available OpenCL platforms:" << std::endl;
        for (int i = 0; i < platforms.size(); ++i) {
            auto name = platforms[i].getInfo<CL_PLATFORM_NAME>();
            text << "    " << i + 1 << ": " << name.c_str() << std::endl;
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
             << platform_name.c_str() << std::endl << std::endl;
        for (int i = 0; i < devices.size(); i++) {
            auto name = devices[i].getInfo<CL_DEVICE_NAME>();
            text << "\t" << i + 1 << ": "
                 << name.c_str() << std::endl;
            text << "\t\tMax compute units: "
                 << devices[i].getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>() << std::endl;
            text << "\t\tMax work group size: "
                 << devices[i].getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>() << std::endl << std::endl;
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
        std::string source = readFile("lib/shaders/raytracing.cl");
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
        testKernel = cl::Kernel(program, "test_kernel");
        rayGenerationKernel = cl::Kernel(program, "raygeneration_kernel");
        renderKernel = cl::Kernel(program, "render_kernel");
        programInited = true;
    }
    if (width != _width || height != _height) {
        _width = width;
        _height = height;
        // prepare buffers
        frameBuffer.resize(_width * _height * 4);
        std::fill(frameBuffer.begin(), frameBuffer.end(), 0.5f);
        rayBuffer = cl::Buffer(context, CL_MEM_READ_WRITE, _width * _height * sizeof(Ray), nullptr);
        int err;
        testBuffer = cl::Buffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, frameBufferSize(),
            frameBuffer.data(), &err);
        if (err) {
            fprintf(stderr, "Error creaing buffer: %d\n", err);
        }
        std::fill(frameBuffer.begin(), frameBuffer.end(), 1.0f);
        testKernel.setArg(0, testBuffer());
        testKernel.setArg(1, width);
        testKernel.setArg(2, height);
    }
    return true;
}

void cg::BVH::buildFromTriangles(std::vector<Triangle> &triangles) {
    nodes.clear();
    const auto push = [&](auto &&...args) -> uint {
        uint ret = static_cast<uint>(nodes.size());
        nodes.emplace_back(std::forward<decltype(args)>(args)...);
        return ret;
    };
    using Iter = std::vector<Triangle>::iterator;
    const std::function<uint(Iter, Iter, uint)> recur = [&](Iter first, Iter last, uint baseIndex) -> uint {
        auto length = last - first;
        if (length == 1) {
            return push(BVHNode{first->bounds(), baseIndex, 1}); // leaf
        }
        if (length == 2) {
            auto bounds = first->bounds() + std::next(first)->bounds();
            unsigned short dim = bounds.maxExtent();
            auto ret = push(BVHNode{bounds, 0, 0, dim});
            if (first->bounds().centroid().s[dim] < (std::next(first))->bounds().centroid().s[dim]) {
                recur(first, std::next(first), baseIndex);                         // left
                nodes[ret].offset = recur(std::next(first), last, baseIndex + 1);  // right
            } else {
                recur(std::next(first), last, baseIndex + 1);                      // right
                nodes[ret].offset = recur(first, std::next(first), baseIndex + 1); // left
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
        auto ret = push(BVHNode{bounds, 0, 0, maxExtend});
        recur(first, first + leftSize, baseIndex);                                  // left
        nodes[ret].offset = recur(first + leftSize, last, baseIndex + leftSize);    // right
        return ret;
    };
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
    glUniform1i(glGetUniformLocation(shader.id, "screenTexture"), 0);
}

cg::RayTracingRenderer::ShaderParams::~ShaderParams() {
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
}

void cg::RayTracingRenderer::ShaderParams::use() const {
    shader.use();
    glBindVertexArray(vao);
}
