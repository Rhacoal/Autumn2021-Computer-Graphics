#ifndef ASSIGNMENT_APPLICATION_H
#define ASSIGNMENT_APPLICATION_H

#include <cg_fwd.h>
#include <cg_common.h>
#include <scene.h>
#include <texture.h>

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
    static std::map<std::string, Texture> _managed_textures;
    GLFWwindow *_window{};
    volatile int _running = 1;
    int started = 0;
    Scene _scene;
    int width{}, height{};
public:
    Application() = default;

    Application(const Application &) = delete;

    virtual ~Application() = default;

    virtual void update() {}

    virtual void draw() {}

    virtual void onResize(int, int) {}

    virtual void initScene() {}

    virtual void cleanUp() {}

    int windowWidth() const { return width; }

    int windowHeight() const { return height; }

    Scene &currentScene();

    void start(const ApplicationConfig &config);

    void terminate();

    GLFWwindow *window() const { return _window; }

    bool running() const { return _running; }

    static Texture loadTexture(const char *path, Texture::Encoding encoding);
};
}

#endif //ASSIGNMENT_APPLICATION_H
