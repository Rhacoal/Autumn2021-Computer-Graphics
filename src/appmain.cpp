#include <application.h>
#include <renderer.h>
#include <camera.h>
#include <material.h>
#include <geometry.h>
#include <mesh.h>
#include <lighting.h>
#include <obj_loader.h>
#include <rt.h>
#include <helper/axis_helper.h>

#include "lib/shaders/rt_structure.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <perlin_noise.hpp>
#include <tinyfiledialogs.h>

#include <iostream>

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

class AppMain : public cg::Application {
    inline static AppMain *appMain = nullptr;
    Renderer renderer;
    PerspectiveCamera camera;
    std::optional<ShaderPass> invert, gray, gammaCorrection, trivial;
    std::optional<ShaderPassLink> shaderPasses;
    std::optional<RayTracingRenderer> rtRenderer;
    RayTracingScene rtRendererScene;

    // objects
    Skybox *skybox = nullptr;
    DirectionalLight *dirLight;
    Object3D *loadedObject = nullptr;
    Mesh *highlightObject = nullptr;

    // mouse
    double lastMouseX = .0, lastMouseY = .0;
    double deltaX = .0, deltaY = .0;
    bool hasMouseMove = false;
    bool showCursor = false;
    bool rightButtonClicked = false;

    // shoot object
    Mesh *bullet = nullptr;
    int bulletLife = 0;
    static constexpr float bulletSpeed = 5 / 60.f;
    static constexpr int bulletMaxLife = 60 * 60;
public:
    bool cpuRendering = false;
    static constexpr float camera_dist = 4.f / 1.414f;

    AppMain() : camera(45, .1, 500, 16 / 9.f) {
        if (appMain) {
            throw std::runtime_error("this application should be run in singleton mode!");
        }
        appMain = this;
    }

    void initScene() override {
        // post effect
        invert.emplace(inverse_frag, windowWidth(), windowHeight());
        gray.emplace(gray_frag, windowWidth(), windowHeight());
        gammaCorrection.emplace(gamma_correction_frag, windowWidth(), windowHeight());
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
        currentScene().addChild(new AmbientLight(glm::vec3(1.0f, 1.0f, 1.0f), .1f));
        dirLight = new DirectionalLight(glm::vec3(0.45f, -0.48f, 0.75f), glm::vec3(1.0f, 1.0f, 1.0f), 2.f);
        currentScene().addChild(dirLight);

        // camera
        camera.setPosition(glm::vec3(5.0f, 5.0f, 5.0f));
        camera.lookAt(glm::vec3(0.0f, 0.0f, 0.0f));

        // bullet
        bullet = new Mesh(std::make_shared<PhongMaterial>(), std::make_shared<BoxGeometry>(0.5f, 0.5f, 5.0f));
        auto sphere = new Mesh(std::make_shared<PhongMaterial>(), std::make_shared<SphereGeometry>(0.5f, 20, 20));
        sphere->material()->emission = glm::vec4{0.0f, 0.0f, 0.0f, 0.0f};
        sphere->material()->color = glm::vec4{1.0f, 0.0f, 0.0f, 0.0f};
//        auto box = new Mesh(std::make_shared<PhongMaterial>(), std::make_shared<BoxGeometry>(1.0f, 1.0f, 1.0f));
        auto box = new Mesh(std::make_shared<PhongMaterial>(), std::make_shared<SphereGeometry>(1.0f, 20, 20));
        box->setPosition(glm::vec3(3.5f, 0.0f, 0.0f));
        box->material()->emission = glm::vec4{0.0f, 0.0f, 0.0f, 0.0f};
        box->material()->color = glm::vec4{0.0f, 1.0f, 0.0f, 0.0f};
        auto lightBox = new Mesh(std::make_shared<PhongMaterial>(), std::make_shared<BoxGeometry>(1.0f, 1.0f, 1.0f));
        lightBox->setPosition(glm::vec3(0.75f, 0.0f, -1.5f));
        lightBox->material()->emission = glm::vec4{2.0f, 5.0f, 2.0f, 0.0f};
        lightBox->material()->color = glm::vec4{1.0f, 1.0f, 1.0f, 0.0f};
//        auto bigBox = new Mesh(std::make_shared<PhongMaterial>(), std::make_shared<BoxGeometry>(20.0f, 20.0f, 20.0f));
        currentScene().addChild(sphere);
        currentScene().addChild(box);
        currentScene().addChild(lightBox);
//        currentScene().addChild(bigBox);

        // axis helper
        currentScene().addChild(new AxisHelper({1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, 5));

        // control
        glfwSetInputMode(window(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        glfwSetCursorPosCallback(window(), [](GLFWwindow *, double mouseX, double mouseY) {
            auto &app = *appMain;
            if (app.showCursor) {
                app.hasMouseMove = false;
                app.lastMouseX = mouseX;
                app.lastMouseY = mouseY;
                return;
            }
            if (!app.hasMouseMove) {
                app.lastMouseX = mouseX;
                app.lastMouseY = mouseY;
                app.hasMouseMove = true;
            }
            app.deltaX = mouseX - app.lastMouseX;
            app.deltaY = mouseY - app.lastMouseY;
            app.lastMouseX = mouseX;
            app.lastMouseY = mouseY;
        });
        glfwSetMouseButtonCallback(window(), [](GLFWwindow *, int button, int action, int mods) {
            printf("click %d %d\n", button, action);
            if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE) {
                puts("rclicked");
                appMain->rightButtonClicked = true;
            }
        });

        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO &io = ImGui::GetIO();

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
        static bool use_ray_tracing = false;
        if (use_skybox) {
            currentScene().addChild(skybox);
        } else {
            currentScene().removeChild(skybox);
        }

        shaderPasses->usePasses({
            {use_invert, &invert.value()},
            {use_gray,   &gray.value()},
            {use_gamma,  &gammaCorrection.value()},
        });

        shaderPasses->renderBegin();
        if (use_ray_tracing) {
            if (cpuRendering) {
                rtRenderer->renderCPU(rtRendererScene, camera);
            } else {
                rtRenderer->render(rtRendererScene, camera);
            }
        } else {
            renderer.render(currentScene(), camera);
        }
        shaderPasses->renderEnd();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        {
            static float f = 0.0f;
            static int counter = 0;
            static float clear_color[3]{};

            ImGui::Begin("Assignment 3");
            if (ImGui::Button("load scene...")) {
                static constexpr const char *filter[] = {"*.json"};
                const char *path = tinyfd_openFileDialog("Load Scene", "", 1, filter, "scene file", 0);
                if (path) {
                    auto obj = loadJsonScene(path);
                    if (!obj) {
                        char buf[1024];
                        snprintf(buf, 1024, "Failed to load scene file: %s", path);
                        tinyfd_messageBox("Load Scene", buf, "ok", "error", 1);
                    } else {
                        if (loadedObject) {
                            currentScene().removeChild(loadedObject);
                            delete loadedObject;
                            highlightObject = nullptr;
                        }
                        loadedObject = obj;
                        currentScene().addChild(obj);
                    }
                }
            }

            if (ImGui::Button(use_ray_tracing ? "disable ray tracing" : "enable ray tracing")) {
                if (use_ray_tracing) {
                    use_ray_tracing = !use_ray_tracing;
                } else {
                    if (!rtRenderer.has_value()) {
                        rtRenderer.emplace();
                    }
                    rtRenderer->init(640, 360);
                    rtRendererScene.setFromScene(currentScene());
                    puts("ray tracing set");
                    use_ray_tracing = true;
                }
            }

            if (ImGui::Button("reload shader")) {
                if (rtRenderer.has_value()) {
                    rtRenderer.value().reloadShader();
                }
            }

            if (ImGui::Button("debug")) {
                float pixel_x = 0.0f;
                float pixel_y = 0.0f;
                float near = camera.near();
                float fov = camera.fov() / 180.f * math::pi<float>();
                float ndc_x = (pixel_x / windowWidth()) * 2.0f - 1.0f;
                float ndc_y = (pixel_y / windowHeight()) * 2.0f - 1.0f;
                float aspect = (float) windowWidth() / (float) windowHeight();
                float top = near * tan(fov / 2);
                auto near_pos = glm::vec3{ndc_x * top * aspect, ndc_y * top, -near};
                auto cameraX = glm::cross(camera.lookDir(), camera.up());
                auto cameraY = glm::cross(cameraX, camera.lookDir());
                auto cameraZ = -camera.lookDir();
                auto world_pos = near_pos.x * cameraX + near_pos.y * cameraY + near_pos.z * cameraZ + camera.position();
                currentScene().addChild(
                    new Mesh(std::make_shared<PhongMaterial>(), std::make_shared<SphereGeometry>(0.05f)));
            }

            ImGui::Text("filters");

            ImGui::Checkbox("invert color", &use_invert);
            ImGui::Checkbox("grayscale", &use_gray);
            ImGui::Checkbox("gamma correction", &use_gamma);

            ImGui::Text("options");
            ImGui::Checkbox("skybox", &use_skybox);


            ImGui::Text("spp: %d", rtRenderer.has_value() ? rtRenderer.value().sampleCount() : 0);
            ImGui::Text("mouse: (%.2f, %.2f)", lastMouseX, lastMouseY);
            ImGui::Text("average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate,
                ImGui::GetIO().Framerate);
            ImGui::Text("camera: pos(%.2f, %.2f, %.2f) dir(%.2f, %.2f, %.2f)",
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

    void updateCamera() {
        constexpr float cameraSpeed = 0.2f;
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
        if (glfwGetKey(window(), GLFW_KEY_Z) == GLFW_PRESS) { // down
            camera.setPosition(camera.position() - cameraSpeed * up);
        }

        if (glfwGetKey(window(), GLFW_KEY_UP) == GLFW_PRESS) { // pitch up
            pitch = std::max(cameraMinPitch, pitch + cameraPitchSpeed);
        }
        if (glfwGetKey(window(), GLFW_KEY_DOWN) == GLFW_PRESS) { // pitch down
            pitch = std::min(cameraMaxPitch, pitch - cameraPitchSpeed);
        }
        if (glfwGetKey(window(), GLFW_KEY_LEFT) == GLFW_PRESS) { // yaw left
            yaw += cameraYawSpeed;
        }
        if (glfwGetKey(window(), GLFW_KEY_RIGHT) == GLFW_PRESS) { // yaw right
            yaw -= cameraYawSpeed;
        }
        glm::vec3 direction;
        direction.x = cos(yaw) * sin(pitch);
        direction.y = cos(pitch);
        direction.z = sin(yaw) * sin(pitch);
        direction = worldToCamera * direction;
        camera.lookAt(camera.position() + direction);
    }

    void updateMouse() {
        if (hasMouseMove && (abs(deltaX) > 1e-2 || abs(deltaY) > 1e-2)) {
            deltaX /= windowWidth();
            deltaY /= windowHeight();
            const auto up = camera.up(); // +y
            const auto dir = glm::normalize(camera.lookDir());
            const auto right = glm::normalize(glm::cross(dir, up)); // -x
            const auto left = -right; // +x
            const auto front = glm::normalize(glm::cross(up, right)); // -x
            const auto back = -front; // +z
            glm::mat3 worldToCamera{left, up, back};
            glm::mat3 cameraToWorld = glm::inverse(worldToCamera);

            constexpr float cameraPitchSpeed = math::radians(-90);
            constexpr float cameraYawSpeed = math::radians(180);
            constexpr float cameraMinPitch = math::radians(10), cameraMaxPitch = math::radians(170);
            float pitch = acos(glm::dot(up, glm::normalize(dir))); // [0, 180)
            const auto projection = cameraToWorld * (dir - glm::dot(dir, up) * up);

            float yaw = atan2(projection.z, projection.x);
            pitch = std::clamp(pitch - (float) deltaY * cameraPitchSpeed, cameraMinPitch, cameraMaxPitch);
            yaw -= cameraYawSpeed * (float) deltaX;
            glm::vec3 direction;
            direction.x = cos(yaw) * sin(pitch);
            direction.y = cos(pitch);
            direction.z = sin(yaw) * sin(pitch);
            direction = worldToCamera * direction;
            camera.lookAt(camera.position() + direction);

            deltaX = deltaY = 0.0;
        }
        if (rightButtonClicked) {
            double mouse_x = showCursor ? lastMouseX : windowWidth() / 2.0;
            double mouse_y = showCursor ? lastMouseY : windowHeight() / 2.0;
            double x = (mouse_x / windowWidth()) * 2.0 - 1.0;
            double y = (mouse_y / windowHeight()) * -2.0 + 1.0;
            double z = -1.0;
            auto wpos = glm::inverse(camera.projectionMatrix() * camera.viewMatrix()) * glm::vec4(x, y, z, 1.0f);
            auto world_pos = glm::vec3(wpos / wpos.w);
            util::printVec(world_pos);
            auto dir = glm::normalize(world_pos - camera.worldPosition());
            util::printVec(dir);
            bullet->setPosition(camera.worldPosition() + dir * 5.0f);
            bullet->lookToDir(dir);
            currentScene().addChild(bullet);
            bulletLife = bulletMaxLife;
            rightButtonClicked = false;
        }
        // update showCursor here since delta would not be have been calculated until next update
        if (glfwGetKey(window(), GLFW_KEY_LEFT_ALT) == GLFW_PRESS ||
            glfwGetKey(window(), GLFW_KEY_RIGHT_ALT) == GLFW_PRESS) {
            if (!showCursor) {
                glfwSetInputMode(window(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                glfwSetCursorPos(window(), windowWidth() / 2.0, windowHeight() / 2.0);
                showCursor = true;
            }
        } else {
            if (showCursor) {
                glfwSetInputMode(window(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                showCursor = false;
            }
        }
    }

    void setHighlightObject(Mesh *mesh) {
        static glm::vec4 prevColor{};
        if (highlightObject == mesh) { return; }
        if (highlightObject) {
            highlightObject->material()->color = prevColor;
        }
        highlightObject = mesh;
        if (!mesh) { return; }
        prevColor = mesh->material()->color;
        mesh->material()->color = glm::vec4{2.0f, 0.3f, 0.3f, 1.0f};
    }

    void updateBullet() {
        if (bullet && bulletLife > 0) {
            bulletLife -= 1;
            if (bulletLife == 0) {
                currentScene().removeChild(bullet);
                return;
            }
            bullet->setPosition(bullet->position() + bullet->lookDir() * bulletSpeed);
            auto bullet_bounding = bullet->computeBoundingBox();
            Mesh *selected = nullptr;
            currentScene().traverse([&](Object3D &obj) {
                Mesh *mesh = obj.isMesh();
                if (!mesh || mesh == bullet) return;
                auto mesh_bounding = mesh->computeBoundingBox();
                if (mesh_bounding.intersect(bullet_bounding)) {
                    selected = mesh;
                }
            });
            setHighlightObject(selected);
        }
    }

    void update() override {
        auto &io = ImGui::GetIO();
        updateCamera();
        updateMouse();
        updateBullet();
    }

    void cleanUp() override {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }
};

int main(int argc, const char **argv) {
    AppMain app;
    for (int i = 0; i < argc; ++i) {
        // cpu rendering
        if (strcmp(argv[i], "cpu") == 0) {
            app.cpuRendering = true;
        }
    }
    app.start(cg::ApplicationConfig{
        .window =  {
            .width = 1024, .height = 576,
            .title = "Assignment 2"
        }
    });
    return 0;
}
