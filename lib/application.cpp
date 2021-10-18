#include <application.h>
#include <material.h>
#include <scene.h>

#include <stb_image.h>

#include <cstdio>
#include <chrono>
#include <algorithm>
#include <numeric>

void cg::Application::terminate() {
    _running = 0;
    // Close OpenGL window and terminate GLFW
    glfwTerminate();
}

cg::Scene &cg::Application::currentScene() {
    return _scene;
}

using namespace std::chrono_literals;

struct MA {
    constexpr static int MAX = 50;
    std::optional<decltype(std::chrono::steady_clock::now())> tp;
    double intervals[MAX] = {};
    int ptr = 0;

    void tick() {
        auto t1 = std::chrono::steady_clock::now();
        if (tp.has_value()) {
            intervals[ptr] = std::chrono::duration<double, std::milli>(t1 - tp.value()).count();
            ptr = (ptr + 1) % MAX;
        }
        tp = t1;
    }

    void track(double t) {
        tp.reset();
        intervals[(ptr++) % MAX] = t;
    }

    double average() {
        return 1000.0 / std::accumulate(intervals, intervals + MAX, 0.0) * MAX;
    }
};

cg::Texture cg::Application::loadTexture(const char *path) {
    // if loaded, simply returns it
    auto it = _managed_textures.find(path);
    std::string key = path;
    if (it != _managed_textures.end()) {
        return it->second;
    }

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    // set the texture wrapping/filtering options (on the currently bound texture object)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // load and generate the texture
    int width, height, nrChannels;
    unsigned char *data = stbi_load(path, &width, &height, &nrChannels, STBI_rgb_alpha);
    if (data) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        stbi_image_free(data);
    } else {
        // fail silently
        fprintf(stderr, "failed to load texture %s\n", path);
    }
    return Texture(tex);
}

static bool frameBufferResized = false;

void framebufferSizeCallback(GLFWwindow *window, int width, int height) {
    frameBufferResized = true;
    glViewport(0, 0, width, height);
}

void cg::Application::start(const ApplicationConfig &config) {
    if (started) {
        return;
    }
    // Initialise GLFW
    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        _running = 0;
        return;
    }

    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Open a window and create its OpenGL context
    _window = glfwCreateWindow(config.window.width, config.window.height, config.window.title, nullptr, nullptr);
    if (_window == nullptr) {
        fprintf(stderr, "Failed to open GLFW window.\n");
        getchar();
        glfwTerminate();
        _running = 0;
        return;
    }
    glfwMakeContextCurrent(_window);
    glfwGetFramebufferSize(_window, &width, &height);

    if (!initGL()) {
        fprintf(stderr, "Failed to initialize GLEW\n");
        getchar();
        glfwTerminate();
        _running = 0;
        return;
    }

    printf("glGenVertexArrays: %p\n", glGenVertexArrays);
    printf("glShaderSource: %p\n", glShaderSource);

    // Ensure we can capture the escape key being pressed below
    glfwSetInputMode(_window, GLFW_STICKY_KEYS, GL_TRUE);

    glfwSetFramebufferSizeCallback(_window, framebufferSizeCallback);

    // main app loop
    initScene();

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    MA frame_ma;
    int ticks = 0;
    do {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (frameBufferResized) {
            glfwGetFramebufferSize(_window, &width, &height);
            onResize(width, height);
        }
        update();
        draw();

        frame_ma.tick();
        if ((++ticks) % 100 == 0) {
            printf("average: %lf\n", frame_ma.average());
            ticks = 0;
        }

        // Swap buffers
        glfwSwapBuffers(_window);
        glfwPollEvents();

    } while (running() && glfwWindowShouldClose(_window) == 0);

    cleanUp();
    terminate();
}
