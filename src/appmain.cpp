#include <application.h>
#include <renderer.h>
#include <camera.h>
#include <material.h>
#include <geometry.h>
#include <mesh.h>

using namespace cg;

class AppMain : public cg::Application {
    Renderer renderer;
    PerspectiveCamera camera;
public:
    AppMain() : camera(75, 1, 2000, 16 / 9.0f) {}

    void initScene() override {
        Mesh *box = new Mesh(std::make_shared<PhongMaterial>(), std::make_shared<BoxGeometry>(1.0f, 1.0f, 1.0f));
        currentScene().addChild(box);
    }

    void draw() override {
        renderer.render(currentScene(), camera);
    }
};

int main() {
    AppMain app;
    app.start(cg::ApplicationConfig{
        .window =  {
            .width =  800, .height = 450,
            .title =  "test"
        }
    });
    return 0;
}
