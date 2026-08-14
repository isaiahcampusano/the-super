#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <iostream>
#include <memory>
#include <numbers>
#include <stdexcept>
#include <string_view>
#include <vector>

#include <glad/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "the_super/Camera.hpp"
#include "the_super/PointCloudRenderer.hpp"
#include "the_super/ShaderProgram.hpp"

namespace {

constexpr int initialWidth = 1280;
constexpr int initialHeight = 800;

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
};

struct Mesh {
    GLuint vertexArray {};
    GLuint vertexBuffer {};
    GLuint indexBuffer {};
    GLsizei indexCount {};
};

struct Nucleon {
    glm::vec3 position;
    bool proton;
};

constexpr std::array<Nucleon, 7> placeholderNucleus {{
    {{0.0F, 0.0F, 0.0F}, true},
    {{0.9F, 0.0F, 0.0F}, false},
    {{-0.9F, 0.0F, 0.0F}, false},
    {{0.0F, 0.9F, 0.0F}, true},
    {{0.0F, -0.9F, 0.0F}, true},
    {{0.0F, 0.0F, 0.9F}, false},
    {{0.0F, 0.0F, -0.9F}, true},
}};

struct AppState {
    the_super::OrbitCamera camera;
    float resetDistance {9.0F};
    double lastCursorX {};
    double lastCursorY {};
    bool cursorInitialized {false};
};

struct SceneSettings {
    float nucleonScale {0.62F};
    float animationSpeed {0.8F};
    float guidePointSize {4.0F};
    bool animate {true};
    bool showGuidePoints {true};
};

struct GlfwSession {
    ~GlfwSession() {
        glfwTerminate();
    }
};

class ImGuiSession {
public:
    explicit ImGuiSession(GLFWwindow* window) {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsDark();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.IniFilename = nullptr;
        if (!ImGui_ImplGlfw_InitForOpenGL(window, true)) {
            ImGui::DestroyContext();
            throw std::runtime_error("Could not initialize Dear ImGui GLFW backend");
        }
        if (!ImGui_ImplOpenGL3_Init("#version 330 core")) {
            ImGui_ImplGlfw_Shutdown();
            ImGui::DestroyContext();
            throw std::runtime_error("Could not initialize Dear ImGui OpenGL backend");
        }
    }

    ~ImGuiSession() {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }

    ImGuiSession(const ImGuiSession&) = delete;
    ImGuiSession& operator=(const ImGuiSession&) = delete;
};

bool imguiCapturesMouse() {
    return ImGui::GetCurrentContext() != nullptr && ImGui::GetIO().WantCaptureMouse;
}

bool imguiCapturesKeyboard() {
    return ImGui::GetCurrentContext() != nullptr && ImGui::GetIO().WantCaptureKeyboard;
}

void glfwErrorCallback(int error, const char* description) {
    std::cerr << "GLFW error " << error << ": " << description << '\n';
}

void framebufferSizeCallback(GLFWwindow*, int width, int height) {
    glViewport(0, 0, width, height);
}

void keyCallback(GLFWwindow* window, int key, int, int action, int) {
    if (action != GLFW_PRESS || imguiCapturesKeyboard()) {
        return;
    }
    if (key == GLFW_KEY_ESCAPE) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    } else if (key == GLFW_KEY_R) {
        auto* state = static_cast<AppState*>(glfwGetWindowUserPointer(window));
        state->camera.reset(state->resetDistance);
    }
}

void cursorPositionCallback(GLFWwindow* window, double x, double y) {
    auto* state = static_cast<AppState*>(glfwGetWindowUserPointer(window));
    if (!state->cursorInitialized) {
        state->lastCursorX = x;
        state->lastCursorY = y;
        state->cursorInitialized = true;
        return;
    }

    const float deltaX = static_cast<float>(x - state->lastCursorX);
    const float deltaY = static_cast<float>(y - state->lastCursorY);
    state->lastCursorX = x;
    state->lastCursorY = y;
    if (imguiCapturesMouse()) {
        return;
    }

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        state->camera.orbit(deltaX, deltaY);
    } else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
        state->camera.pan(deltaX, deltaY);
    }
}

void scrollCallback(GLFWwindow* window, double, double yOffset) {
    if (imguiCapturesMouse()) {
        return;
    }
    auto* state = static_cast<AppState*>(glfwGetWindowUserPointer(window));
    state->camera.zoom(static_cast<float>(yOffset));
}

Mesh createSphere(unsigned int latitudeSegments, unsigned int longitudeSegments) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    vertices.reserve(
        static_cast<std::size_t>(latitudeSegments + 1U)
        * static_cast<std::size_t>(longitudeSegments + 1U)
    );

    for (unsigned int latitude = 0; latitude <= latitudeSegments; ++latitude) {
        const float v = static_cast<float>(latitude) / static_cast<float>(latitudeSegments);
        const float polar = v * std::numbers::pi_v<float>;
        for (unsigned int longitude = 0; longitude <= longitudeSegments; ++longitude) {
            const float u = static_cast<float>(longitude)
                / static_cast<float>(longitudeSegments);
            const float azimuth = u * 2.0F * std::numbers::pi_v<float>;
            const glm::vec3 normal {
                std::sin(polar) * std::cos(azimuth),
                std::sin(polar) * std::sin(azimuth),
                std::cos(polar),
            };
            vertices.push_back(Vertex {normal, normal});
        }
    }

    for (unsigned int latitude = 0; latitude < latitudeSegments; ++latitude) {
        for (unsigned int longitude = 0; longitude < longitudeSegments; ++longitude) {
            const unsigned int rowLength = longitudeSegments + 1U;
            const unsigned int topLeft = (latitude * rowLength) + longitude;
            const unsigned int bottomLeft = topLeft + rowLength;
            indices.insert(
                indices.end(),
                {topLeft, bottomLeft, topLeft + 1U, topLeft + 1U, bottomLeft, bottomLeft + 1U}
            );
        }
    }

    Mesh mesh;
    mesh.indexCount = static_cast<GLsizei>(indices.size());
    glGenVertexArrays(1, &mesh.vertexArray);
    glGenBuffers(1, &mesh.vertexBuffer);
    glGenBuffers(1, &mesh.indexBuffer);
    glBindVertexArray(mesh.vertexArray);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vertexBuffer);
    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)),
        vertices.data(),
        GL_STATIC_DRAW
    );
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.indexBuffer);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned int)),
        indices.data(),
        GL_STATIC_DRAW
    );
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        static_cast<GLsizei>(sizeof(Vertex)),
        reinterpret_cast<void*>(offsetof(Vertex, position))
    );
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1,
        3,
        GL_FLOAT,
        GL_FALSE,
        static_cast<GLsizei>(sizeof(Vertex)),
        reinterpret_cast<void*>(offsetof(Vertex, normal))
    );
    glBindVertexArray(0);
    return mesh;
}

void destroy(const Mesh& mesh) {
    glDeleteBuffers(1, &mesh.indexBuffer);
    glDeleteBuffers(1, &mesh.vertexBuffer);
    glDeleteVertexArrays(1, &mesh.vertexArray);
}

std::filesystem::path shaderDirectory(const char* executablePath) {
    return std::filesystem::absolute(executablePath).parent_path() / "assets" / "shaders";
}

std::array<the_super::PointSample, placeholderNucleus.size()> guidePoints() {
    std::array<the_super::PointSample, placeholderNucleus.size()> points {};
    for (std::size_t index = 0; index < placeholderNucleus.size(); ++index) {
        const Nucleon& nucleon = placeholderNucleus[index];
        points[index] = {nucleon.position * 1.8F, 1.0F, nucleon.proton ? 0.0F : 1.0F};
    }
    return points;
}

void drawControls(SceneSettings& settings, AppState& state) {
    ImGui::SetNextWindowPos(ImVec2(18.0F, 18.0F), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(330.0F, 0.0F), ImGuiCond_FirstUseEver);
    ImGui::Begin("The Super", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::TextUnformatted("Nuclear visualization foundation");
    ImGui::Separator();
    ImGui::Text("Placeholder nucleus: %zu nucleons", placeholderNucleus.size());
    ImGui::SliderFloat("Nucleon size", &settings.nucleonScale, 0.25F, 1.0F, "%.2f");
    ImGui::Checkbox("Animate", &settings.animate);
    ImGui::SliderFloat("Animation speed", &settings.animationSpeed, 0.0F, 3.0F, "%.2f");
    ImGui::Checkbox("Show guide points", &settings.showGuidePoints);
    if (settings.showGuidePoints) {
        ImGui::SliderFloat("Guide point size", &settings.guidePointSize, 1.0F, 12.0F, "%.1f");
    }
    if (ImGui::Button("Reset camera")) {
        state.camera.reset(state.resetDistance);
    }
    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.96F, 0.28F, 0.20F, 1.0F), "Red: proton");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.20F, 0.52F, 1.0F, 1.0F), "Blue: neutron");
    ImGui::TextUnformatted("Left drag: orbit   Right drag: pan");
    ImGui::TextUnformatted("Wheel: zoom   R: reset   Esc: exit");
    ImGui::End();
}

int run(const char* executablePath, bool smokeTest) {
    glfwSetErrorCallback(glfwErrorCallback);
    if (glfwInit() != GLFW_TRUE) {
        throw std::runtime_error("Could not initialize GLFW");
    }
    const GlfwSession glfwSession;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    if (smokeTest) {
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    }
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif

    const auto window = std::unique_ptr<GLFWwindow, decltype(&glfwDestroyWindow)> {
        glfwCreateWindow(initialWidth, initialHeight, "The Super", nullptr, nullptr),
        glfwDestroyWindow,
    };
    if (window == nullptr) {
        throw std::runtime_error("Could not create an OpenGL 3.3 window");
    }

    glfwMakeContextCurrent(window.get());
    glfwSwapInterval(1);
    if (gladLoadGL(glfwGetProcAddress) == 0) {
        throw std::runtime_error("Could not load OpenGL functions with GLAD");
    }

    AppState state;
    state.camera.reset(state.resetDistance);
    glfwSetWindowUserPointer(window.get(), &state);
    glfwSetFramebufferSizeCallback(window.get(), framebufferSizeCallback);
    glfwSetKeyCallback(window.get(), keyCallback);
    glfwSetCursorPosCallback(window.get(), cursorPositionCallback);
    glfwSetScrollCallback(window.get(), scrollCallback);
    const ImGuiSession imgui(window.get());

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    const std::filesystem::path shaders = shaderDirectory(executablePath);
    the_super::ShaderProgram sphereShader(shaders / "nucleus.vert", shaders / "nucleus.frag");
    the_super::PointCloudRenderer pointRenderer(shaders);
    const auto points = guidePoints();
    pointRenderer.upload(points);
    const Mesh sphere = createSphere(28U, 40U);
    SceneSettings settings;

    int completedFrames = 0;
    while (glfwWindowShouldClose(window.get()) == GLFW_FALSE) {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        drawControls(settings, state);

        int framebufferWidth = 0;
        int framebufferHeight = 0;
        glfwGetFramebufferSize(window.get(), &framebufferWidth, &framebufferHeight);
        const int safeHeight = std::max(framebufferHeight, 1);
        const float aspect = static_cast<float>(framebufferWidth)
            / static_cast<float>(safeHeight);
        const glm::mat4 projection = glm::perspective(
            glm::radians(45.0F),
            aspect,
            0.05F,
            120.0F
        );
        const glm::mat4 view = state.camera.viewMatrix();
        const float elapsed = static_cast<float>(glfwGetTime());
        const float rotation = settings.animate ? elapsed * settings.animationSpeed : 0.0F;
        const glm::mat4 sceneRotation = glm::rotate(
            glm::mat4 {1.0F},
            rotation,
            glm::normalize(glm::vec3 {0.2F, 0.6F, 1.0F})
        );

        glViewport(0, 0, framebufferWidth, framebufferHeight);
        glClearColor(0.004F, 0.006F, 0.016F, 1.0F);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (settings.showGuidePoints) {
            pointRenderer.render(view * sceneRotation, projection, settings.guidePointSize);
        }

        sphereShader.use();
        sphereShader.set("view", view);
        sphereShader.set("projection", projection);
        sphereShader.set("cameraPosition", state.camera.position());
        glBindVertexArray(sphere.vertexArray);
        for (const Nucleon& nucleon : placeholderNucleus) {
            glm::mat4 model = sceneRotation;
            model = glm::translate(model, nucleon.position);
            model = glm::scale(model, glm::vec3 {settings.nucleonScale});
            sphereShader.set("model", model);
            sphereShader.set(
                "baseColor",
                nucleon.proton
                    ? glm::vec3 {0.96F, 0.18F, 0.12F}
                    : glm::vec3 {0.18F, 0.48F, 1.0F}
            );
            glDrawElements(GL_TRIANGLES, sphere.indexCount, GL_UNSIGNED_INT, nullptr);
        }

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window.get());

        if (smokeTest && ++completedFrames >= 3) {
            glfwSetWindowShouldClose(window.get(), GLFW_TRUE);
        }
    }

    destroy(sphere);
    if (smokeTest) {
        std::cout << "smoke-test: rendered " << placeholderNucleus.size()
                  << " placeholder nucleons\n";
    }
    return 0;
}

} // namespace

int main(int argc, char* argv[]) {
    try {
        const char* executablePath = argc > 0 ? argv[0] : "the-super";
        const bool smokeTest = argc > 1 && std::string_view(argv[1]) == "--smoke-test";
        return run(executablePath, smokeTest);
    } catch (const std::exception& exception) {
        std::cerr << "the-super: " << exception.what() << '\n';
        return 1;
    }
}
