#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <owop-client/Camera.hpp>
#include <owop-client/Mouse.hpp>
#include <owop-client/Network.hpp>
#include <owop-client/render/ChunkRenderer.hpp>
#include <owop-client/Constants.hpp>
#include <owop-client/Logger.hpp>
#include <owop-client/Types.hpp>
#include <owop-client/Settings.hpp>

#include <iostream>
#include <stdexcept>
#include <algorithm>

static void glfwErrorCallback(int error, const char* description) {
    owop::Logger::error("GLFW", std::string("Error ") + std::to_string(error) + ": " + description);
}

class OWOPClient {
private:
    GLFWwindow* window;
    owop::Camera camera;
    owop::Mouse mouse;
    bool showTools = true;
    
    owop::Tool currentTool = owop::Tool::Cursor;
    ImVec4 currentColor = ImVec4(0, 0, 0, 1);
    
    owop::Network network;
    owop::ChunkRenderer chunkRenderer;
    
    int windowWidth{800};
    int windowHeight{600};
    
    static void windowResizeCallback(GLFWwindow* window, int width, int height) {
        glViewport(0, 0, width, height);
    }

    void initGLFW() {
        owop::Logger::info("OWOPClient", "Initializing GLFW...");
        if (!glfwInit()) {
            throw std::runtime_error("Failed to initialize GLFW");
        }

        // Request OpenGL 3.3 compatibility profile
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_FALSE);
        glfwWindowHint(GLFW_SAMPLES, 4);  // Enable MSAA
        glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
        glfwWindowHint(GLFW_FOCUSED, GLFW_TRUE);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        owop::Logger::info("OWOPClient", "Creating window...");
        window = glfwCreateWindow(windowWidth, windowHeight, "OWOP Client", nullptr, nullptr);
        if (!window) {
            glfwTerminate();
            throw std::runtime_error("Failed to create GLFW window");
        }

        // Set callbacks
        glfwSetFramebufferSizeCallback(window, windowResizeCallback);
    }

    void initImGui() {
        owop::Logger::info("OWOPClient", "Initializing ImGui...");
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsDark();
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init("#version 130");
    }

    void renderToolsWindow() {
        ImGui::SetNextWindowPos(ImVec2(10, 40), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(200, 300), ImGuiCond_FirstUseEver);
        ImGui::Begin("Tools", nullptr, ImGuiWindowFlags_NoCollapse);
        
        // Tool selection
        const char* tools[] = { "Cursor", "Move", "Pipette" };
        int toolIndex = static_cast<int>(currentTool);
        if (ImGui::Combo("Tool", &toolIndex, tools, IM_ARRAYSIZE(tools))) {
            currentTool = static_cast<owop::Tool>(toolIndex);
        }

        // Color picker with RGB inputs
        ImGui::ColorEdit3("Color", (float*)&currentColor);
        
        // Individual RGB inputs
        int rgb[3] = {
            static_cast<int>(currentColor.x * 255),
            static_cast<int>(currentColor.y * 255),
            static_cast<int>(currentColor.z * 255)
        };
        
        bool colorChanged = false;
        if (ImGui::InputInt("R", &rgb[0])) {
            rgb[0] = std::max(0, std::min(255, rgb[0]));
            currentColor.x = rgb[0] / 255.0f;
            colorChanged = true;
        }
        if (ImGui::InputInt("G", &rgb[1])) {
            rgb[1] = std::max(0, std::min(255, rgb[1]));
            currentColor.y = rgb[1] / 255.0f;
            colorChanged = true;
        }
        if (ImGui::InputInt("B", &rgb[2])) {
            rgb[2] = std::max(0, std::min(255, rgb[2]));
            currentColor.z = rgb[2] / 255.0f;
            colorChanged = true;
        }
        
        ImGui::End();
    }

    void renderSettingsWindow(bool& show) {
        ImGui::SetNextWindowPos(ImVec2(220, 40), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(200, 150), ImGuiCond_FirstUseEver);
        ImGui::Begin("Settings", &show, ImGuiWindowFlags_NoCollapse);
        
        // Server settings
        auto& settings = owop::Settings::getInstance();
        static char serverDomain[128];
        static char worldName[128];
        static bool initialized = false;
        
        if (!initialized) {
            strcpy_s(serverDomain, settings.serverDomain.c_str());
            strcpy_s(worldName, settings.worldName.c_str());
            initialized = true;
        }
        
        if (ImGui::InputText("Server", serverDomain, sizeof(serverDomain))) {
            settings.serverDomain = serverDomain;
        }
        
        if (ImGui::InputText("World", worldName, sizeof(worldName))) {
            settings.worldName = worldName;
        }
        
        if (ImGui::Checkbox("Captcha", &settings.requireCaptcha)) {
            // Setting changed
        }
        
        if (ImGui::Button("Connect")) {
            network.connect("wss://" + settings.serverDomain, settings.worldName);
        }
        
        ImGui::End();
    }

    void renderCoordinates() {
        auto tilePos = mouse.getTilePosition();
        ImGui::SetNextWindowPos(ImVec2(10, 10));
        ImGui::SetNextWindowBgAlpha(0.0f);
        ImGui::Begin("Coordinates", nullptr, 
            ImGuiWindowFlags_NoTitleBar | 
            ImGuiWindowFlags_NoResize | 
            ImGuiWindowFlags_NoMove | 
            ImGuiWindowFlags_NoScrollbar | 
            ImGuiWindowFlags_NoSavedSettings | 
            ImGuiWindowFlags_NoInputs |
            ImGuiWindowFlags_NoBackground);
        ImGui::Text("XY: %d; %d", tilePos.x, tilePos.y);
        ImGui::End();
    }

public:
    OWOPClient() 
        : camera()
        , mouse(camera)
        , chunkRenderer(window)
        , windowWidth(800)
        , windowHeight(600)
    {
        initWindow();
        
        // Load settings
        owop::Settings::getInstance().load();
        
        // Set up chunk data callback
        network.setChunkDataCallback([this](int x, int y, const std::vector<owop::Color>& colors) {
            chunkRenderer.updateChunk(x, y, colors);
        });
    }

    ~OWOPClient() {
        // Cleanup
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        
        if (window) {
            glfwDestroyWindow(window);
        }
        glfwTerminate();
    }

    void run() {
        owop::Logger::info("OWOPClient", "Starting main loop...");

        // Initial chunk request
        network.requestChunksInView(camera.getX(), camera.getY(), camera.getZoom());

        // Main loop
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            renderToolsWindow();

            // Render settings window
            static bool showSettings = true;
            renderSettingsWindow(showSettings);

            renderCoordinates();

            // Clear screen
            glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            // Render chunks
            chunkRenderer.render(camera, windowWidth, windowHeight);

            // Render ImGui
            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            // Swap buffers
            glfwSwapBuffers(window);

            // Handle camera movement
            if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
                ImVec2 delta = ImGui::GetIO().MouseDelta;
                camera.move(-delta.x / camera.getZoom(), -delta.y / camera.getZoom());
                
                // Request chunks for new camera position
                network.requestChunksInView(camera.getX(), camera.getY(), camera.getZoom());
            }

            // Handle zoom
            float wheel = ImGui::GetIO().MouseWheel;
            if (wheel != 0.0f) {
                // Get mouse position before zoom
                ImVec2 mousePos = ImGui::GetMousePos();
                float worldX = (mousePos.x - windowWidth/2) / camera.getZoom() + camera.getX();
                float worldY = (mousePos.y - windowHeight/2) / camera.getZoom() + camera.getY();
                
                // Apply zoom
                camera.zoom(wheel * 0.1f);
                
                // Adjust position to keep mouse point fixed
                camera.moveTo(
                    worldX - (mousePos.x - windowWidth/2) / camera.getZoom(),
                    worldY - (mousePos.y - windowHeight/2) / camera.getZoom()
                );
                
                // Request chunks for new zoom level
                network.requestChunksInView(camera.getX(), camera.getY(), camera.getZoom());
            }
        }
    }

private:
    void initWindow() {
        try {
            owop::Logger::info("OWOPClient", "Starting initialization...");

            // Initialize GLFW and OpenGL
            initGLFW();

            // Make context current
            owop::Logger::info("OWOPClient", "Making OpenGL context current...");
            glfwMakeContextCurrent(window);
            glfwSwapInterval(1);  // Enable vsync

            // Initialize GLAD
            owop::Logger::info("OWOPClient", "Initializing GLAD...");
            if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
                glfwDestroyWindow(window);
                glfwTerminate();
                throw std::runtime_error("Failed to initialize GLAD");
            }

            // Print OpenGL info
            owop::Logger::info("OpenGL", std::string("Version: ") + (const char*)glGetString(GL_VERSION));
            owop::Logger::info("OpenGL", std::string("Vendor: ") + (const char*)glGetString(GL_VENDOR));
            owop::Logger::info("OpenGL", std::string("Renderer: ") + (const char*)glGetString(GL_RENDERER));
            owop::Logger::info("OpenGL", std::string("GLSL Version: ") + (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION));

            // Initialize OpenGL state
            owop::Logger::info("OWOPClient", "Initializing OpenGL state...");
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glEnable(GL_MULTISAMPLE);

            // Initialize ImGui
            owop::Logger::info("OWOPClient", "Initializing ImGui...");
            initImGui();

            owop::Logger::info("OWOPClient", "Initialization complete!");
        } catch (const std::exception& e) {
            owop::Logger::error("OWOPClient", std::string("Initialization failed: ") + e.what());
            throw;
        }
    }
};

int main() {
    try {
        glfwSetErrorCallback(glfwErrorCallback);
        OWOPClient client;
        client.run();
        return 0;
    } catch (const std::exception& e) {
        owop::Logger::error("Main", e.what());
        return 1;
    }
}