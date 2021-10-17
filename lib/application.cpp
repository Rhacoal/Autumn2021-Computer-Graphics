#include <application.h>
#include <material.h>
#include <scene.h>

#include <stb_image.h>

#include <cstdio>

void cg::Application::update() {
    // lets do something
}

void cg::Application::draw() {

}

void cg::Application::initScene() {

}

void cg::Application::terminate() {
    _running = 0;
    // Close OpenGL window and terminate GLFW
    glfwTerminate();
}

cg::Scene &cg::Application::currentScene() {
    return _scene;
}

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

    // main app loop
    initScene();

    // Dark blue background
    glClearColor(0.0f, 0.0f, 1.0f, 0.0f);

    do {
        glClear(GL_COLOR_BUFFER_BIT);

        update();
        draw();

        // Swap buffers
        glfwSwapBuffers(window());
        glfwPollEvents();

    } while (running() && glfwWindowShouldClose(window()) == 0);
}
