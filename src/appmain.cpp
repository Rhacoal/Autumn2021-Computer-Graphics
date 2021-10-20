#include <application.h>
#include <renderer.h>
#include <camera.h>
#include <material.h>
#include <geometry.h>
#include <mesh.h>
#include <lighting.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <perlin_noise.hpp>

using namespace cg;

const char *gray_frag = R"(
#version 330 core
uniform sampler2D screenTexture;
in vec2 texCoord;
out vec4 fragColor;
void main() {
    vec4 color = texture(screenTexture, texCoord);
    float y = 0.299 * color.r + 0.587 * color.g + 0.114 * color.b;
    fragColor = vec4(y, y, y, 1.0);
}
)";

const char *inverse_frag = R"(
#version 330 core
uniform sampler2D screenTexture;
in vec2 texCoord;
out vec4 fragColor;
void main() {
    fragColor = vec4(1.0 - texture(screenTexture, texCoord).rgb, 1.0);
}
)";

const char *gamma_correction_frag = R"(
#version 330 core
uniform sampler2D screenTexture;
in vec2 texCoord;
out vec4 fragColor;
void main() {
    vec4 color = texture(screenTexture, texCoord);
    fragColor = vec4(pow(color.rgb, vec3(1 / 2.2)), color.a);
}
)";

Geometry *generateRandomGround(int segs, float xSize, float zSize, float yScale) {
    siv::BasicPerlinNoise<float> perlin;
    float points[(segs + 1) * (segs + 1) * 3];
    for (int i = 0; i <= segs; ++i) {
        float x = float(i) * 1.0f / xSize;
        for (int j = 0; j <= segs; ++j) {
            float z = float(j) * 1.0f / zSize;
            int index = i * (segs + 1) + j;
            float y = perlin.accumulatedOctaveNoise2D(x, z, 8) * yScale;
            points[index * 3 + 0] = x - xSize / 2;
            points[index * 3 + 1] = y;
            points[index * 3 + 2] = z - zSize / 2;
        }
    }
    unsigned int indices[segs * segs * 3];
    for (int i = 0; i < segs; ++i) {
        for (int j = 0; j < segs; ++j) {
            // TODO generate terrain
        }
    }
    return nullptr;
}

class AppMain : public cg::Application {
    Renderer renderer;
    PerspectiveCamera camera;
    std::optional<ShaderPass> trivial, invert, gray, gamma_correction;
    std::optional<ShaderPassLink> shaderPasses;


    // objects
    Skybox *skybox = nullptr;
    DirectionalLight *dir_light;
public:
    static constexpr float camera_dist = 4.0f / 1.414f;

    AppMain() : camera(45, .1, 100, 16 / 9.0f) {}

    void initScene() override {
        // objects
        auto phong = std::make_shared<PhongMaterial>();
        phong->color = glm::vec4(1.0f, 0.5f, 0.5f, 1.0f);
        Mesh *box = new Mesh(phong, std::make_shared<BoxGeometry>(1.0f, 1.0f, 1.0f));

        auto phong2 = std::make_shared<PhongMaterial>();
        Mesh *box2 = new Mesh(phong2, std::make_shared<BoxGeometry>(1.0f, 1.0f, 1.0f));
        phong2->color = glm::vec4(0.5f, 1.0f, 0.5f, 1.0f);
        box2->setPosition(glm::vec3(1.0f, 0.0f, 0.0f));

        currentScene().addChild(box);
        currentScene().addChild(box2);

        // post effect
        invert.emplace(inverse_frag, windowWidth(), windowHeight());
        gray.emplace(gray_frag, windowWidth(), windowHeight());
        gamma_correction.emplace(gamma_correction_frag, windowWidth(), windowHeight());
        shaderPasses.emplace(windowWidth(), windowHeight());

        // skybox
        const char *skybox_faces[] = {
            "assets/skybox/right.jpg",
            "assets/skybox/left.jpg",
            "assets/skybox/top.jpg",
            "assets/skybox/bottom.jpg",
            "assets/skybox/front.jpg",
            "assets/skybox/back.jpg",
        };
        skybox = new Skybox(skybox_faces);
        currentScene().addChild(skybox);

        // lighting
        currentScene().addChild(new AmbientLight(glm::vec3(1.0f, 1.0f, 1.0f), .0f));
        dir_light = new DirectionalLight(glm::vec3(0.45f, -0.48f, 0.75f), glm::vec3(1.0f, 1.0f, 1.0f), .9f);
        currentScene().addChild(dir_light);

        // camera
        camera.setPosition(glm::vec3(5.0f, 5.0f, 5.0f));
        puts("camera: ");
        camera.lookAt(glm::vec3(0.0f, 0.0f, 0.0f));

        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO &io = ImGui::GetIO();
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();

        // Setup Platform/Renderer backends
        ImGui_ImplGlfw_InitForOpenGL(window(), true);
        ImGui_ImplOpenGL3_Init("#version 330 core");
    }

    void draw() override {
        static bool use_invert = false;
        static bool use_gray = false;
        static bool use_gamma = true;
        static bool use_skybox = true;
        static std::vector<ShaderPass *> passes;
        if (use_skybox) {
            currentScene().addChild(skybox);
        } else if (!use_skybox) {
            currentScene().removeChild(skybox);
        }
        passes.clear();
        if (use_invert) passes.emplace_back(&invert.value());
        if (use_gray) passes.emplace_back(&gray.value());
        if (use_gamma) passes.emplace_back(&gamma_correction.value());
        shaderPasses->usePasses(passes.begin(), passes.end());

        shaderPasses->renderBegin();
        renderer.render(currentScene(), camera);
        shaderPasses->renderEnd();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        {
            static float f = 0.0f;
            static int counter = 0;
            static float clear_color[3]{};

            ImGui::Begin("Assignment 2");

            ImGui::Text("filters");

            ImGui::Checkbox("invert color", &use_invert);
            ImGui::Checkbox("grayscale", &use_gray);
            ImGui::Checkbox("gamma correction", &use_gamma);

            ImGui::Text("options");
            ImGui::Checkbox("skybox", &use_skybox);

            ImGui::Text("average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate,
                        ImGui::GetIO().Framerate);
            ImGui::Text("camera (%.2f, %.2f, %.2f) looking to (%.2f, %.2f, %.2f)",
                        camera.position().x, camera.position().y, camera.position().z,
                        camera.lookDir().x, camera.lookDir().y, camera.lookDir().z);
            ImGui::End();
        }
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    void onResize(int width, int height) override {
        camera.setAspectRatio((float) width / (float) height);
        shaderPasses->resize(width, height);
    }

    void update() override {
        static int cnt = 0;
//        cnt = (cnt + 1) % 360;
//        double theta = cnt / 180.0 * math::pi();
//        camera.setPosition(glm::vec3(cos(theta) * camera_dist, camera_dist, sin(theta) * camera_dist));
//        camera.lookAt(glm::vec3(0.0f, 0.0f, 0.0f));
        constexpr float cameraSpeed = 0.05f; // adjust accordingly
//        camera.setUp(glm::vec3(0.0f, 1.0f, 1.0f));
        const auto up = camera.up(); // +y
        const auto dir = glm::normalize(camera.lookDir());
        const auto right = glm::normalize(glm::cross(dir, up)); // -x
        const auto left = -right; // +x
        const auto front = glm::normalize(glm::cross(up, right)); // -x
        const auto back = -front; // +z
        glm::mat3 worldToCamera{left, up, back};
        glm::mat3 cameraToWorld = glm::inverse(worldToCamera);

        constexpr float cameraPitchSpeed = math::radians(-1);
        constexpr float cameraYawSpeed = math::radians(2);
        constexpr float cameraMinPitch = math::radians(10), cameraMaxPitch = math::radians(170);
        float pitch = acos(glm::dot(up, glm::normalize(dir))); // [0, 180)
        const auto projection = cameraToWorld * (dir - glm::dot(dir, up) * up);
        float yaw = atan2(projection.z, projection.x);
        if (glfwGetKey(window(), GLFW_KEY_W) == GLFW_PRESS) { // front
            camera.setPosition(camera.position() + cameraSpeed * front);
        }
        if (glfwGetKey(window(), GLFW_KEY_A) == GLFW_PRESS) { // left
            camera.setPosition(camera.position() + cameraSpeed * left);
        }
        if (glfwGetKey(window(), GLFW_KEY_S) == GLFW_PRESS) { // back
            camera.setPosition(camera.position() + cameraSpeed * back);
        }
        if (glfwGetKey(window(), GLFW_KEY_D) == GLFW_PRESS) { // right
            camera.setPosition(camera.position() + cameraSpeed * right);
        }
        if (glfwGetKey(window(), GLFW_KEY_Q) == GLFW_PRESS) { // up
            camera.setPosition(camera.position() + cameraSpeed * up);
        }
        if (glfwGetKey(window(), GLFW_KEY_E) == GLFW_PRESS) { // down
            camera.setPosition(camera.position() - cameraSpeed * up);
        }

        if (glfwGetKey(window(), GLFW_KEY_UP) == GLFW_PRESS) { // pitch up
            printf("%f\n", glm::degrees(pitch));
            pitch = std::max(cameraMinPitch, pitch + cameraPitchSpeed);
        }
        if (glfwGetKey(window(), GLFW_KEY_DOWN) == GLFW_PRESS) { // pitch down
            printf("%f\n", glm::degrees(pitch));
            pitch = std::min(cameraMaxPitch, pitch - cameraPitchSpeed);
        }
        if (glfwGetKey(window(), GLFW_KEY_LEFT) == GLFW_PRESS) { // yaw left
            yaw += cameraYawSpeed;
        }
        if (glfwGetKey(window(), GLFW_KEY_RIGHT) == GLFW_PRESS) { // yaw right
            yaw -= cameraYawSpeed;
        }
//        printf("%f, %f\n", yaw, pitch);
        glm::vec3 direction;
        direction.x = cos(yaw) * sin(pitch);
        direction.y = cos(pitch);
        direction.z = sin(yaw) * sin(pitch);
        direction = worldToCamera * direction;
        camera.lookAt(camera.position() + direction);
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
