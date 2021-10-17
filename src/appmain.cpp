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
    static constexpr float camera_dist = 2.0f / 1.414f;

    AppMain() : camera(75, 1, 2000, 16 / 9.0f) {}

    void initScene() override {
        Mesh *box = new Mesh(std::make_shared<PhongMaterial>(), std::make_shared<BoxGeometry>(1.0f, 1.0f, 1.0f));
        currentScene().addChild(box);
    }

    void draw() override {
        renderer.render(currentScene(), camera);
    }

    void onResize(int width, int height) override {
        camera.setAspectRatio((float) width / (float) height);
    }

    void update() override {
        static int cnt = 0;
        cnt = (cnt + 1) % 360;
        double theta = cnt / 180.0 * math::pi();
        camera.setPosition(glm::vec3(cos(theta) * camera_dist, camera_dist, sin(theta) * camera_dist));
        camera.lookAt(glm::vec3(0.0f, 0.0f, 0.0f));
//        const float cameraSpeed = 0.05f; // adjust accordingly
//        if (glfwGetKey(window(), GLFW_KEY_W) == GLFW_PRESS)
//            cameraPos += cameraSpeed * cameraFront;
//        if (glfwGetKey(window(), GLFW_KEY_S) == GLFW_PRESS)
//            cameraPos -= cameraSpeed * cameraFront;
//        if (glfwGetKey(window(), GLFW_KEY_A) == GLFW_PRESS)
//            cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
//        if (glfwGetKey(window(), GLFW_KEY_D) == GLFW_PRESS)
//            cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    }
};

int main() {
    AppMain app;
    app.start(cg::ApplicationConfig{
        .window =  {
            .width = 800, .height = 450,
            .title = "test"
        }
    });
    return 0;
}
