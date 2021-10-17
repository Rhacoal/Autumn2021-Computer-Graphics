#ifndef ASSIGNMENT_APPLICATION_H
#define ASSIGNMENT_APPLICATION_H

#include <cg_common.h>
#include <scene.h>
#include <cg_fwd.h>

#include <memory>
#include <string>
#include <map>
#include <any>

namespace cg {
struct ApplicationConfig {
    // window config
    struct {
        int width, height;
        const char *title;
    } window;
};

class Application {
    std::map<std::string, Texture> _managed_textures;
    GLFWwindow *_window{};
    volatile int _running = 1;
    int started = 0;
    Scene _scene;
public:
    Application() = default;

    Application(const Application &) = delete;

    virtual ~Application() = default;

    virtual void update() {}

    virtual void draw() {}

    virtual void onResize(int width, int height) {}

    virtual void initScene() {}

    Scene &currentScene();

    void start(const ApplicationConfig &config);

    void terminate();

    void setClearColor(float r, float g, float b, float a);

    GLFWwindow *window() const { return _window; }

    bool running() const { return _running; }

    Texture loadTexture(const char *path);
};
}

#endif //ASSIGNMENT_APPLICATION_H
