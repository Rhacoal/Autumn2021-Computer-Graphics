#include <application.h>
#include <renderer.h>
#include <camera.h>
#include <material.h>
#include <geometry.h>
#include <mesh.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

using namespace cg;

const char *inverse_frag = R"(
#version 330 core
uniform sampler2D screenTexture;
in vec2 texCoord;
out vec4 fragColor;
void main() {
    fragColor = vec4(1.0 - texture(screenTexture, texCoord).rgb, 1.0);
}
)";


class AppMain : public cg::Application {
    Renderer renderer;
    PerspectiveCamera camera;
    std::optional<ShaderPass> inverse;
public:
    static constexpr float camera_dist = 2.0f / 1.414f;

    AppMain() : camera(75, 1, 2000, 16 / 9.0f) {}

    void initScene() override {
        Mesh *box = new Mesh(std::make_shared<PhongMaterial>(), std::make_shared<BoxGeometry>(1.0f, 1.0f, 1.0f));
        inverse.emplace(inverse_frag, windowWidth(), windowHeight());
        currentScene().addChild(box);

        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();
        //ImGui::StyleColorsClassic();

        // Setup Platform/Renderer backends
        ImGui_ImplGlfw_InitForOpenGL(window(), true);
        ImGui_ImplOpenGL3_Init("#version 330 core");
    }

    void draw() override {
//        inverse->renderBegin();
        renderer.render(currentScene(), camera);
//        inverse->renderEnd();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        {
            static float f = 0.0f;
            static int counter = 0;
            static float clear_color[3]{};

            ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

            ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)

            ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
            ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

            if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
                counter++;
            ImGui::SameLine();
            ImGui::Text("counter = %d", counter);

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            ImGui::End();
        }
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    void onResize(int width, int height) override {
        camera.setAspectRatio((float) width / (float) height);
        inverse->resize(width, height);
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
    }

    void cleanUp() override {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
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
