#include "GLRenderer.h"
#include <cstring>
#include <exception>
#include <stdexcept>
#include <refl.hpp>

// Define LOG_TAG for this file
#define LOG_TAG "GLRenderer"

GLRenderer::GLRenderer() : program(0), vertexShader(0), 
                          fragmentShader(0), vbo(0), vao(0),
                          imguiInitialized(false), showDemoWindow(true),
                          shaderManager(nullptr), audioManager(nullptr),
                          reflectionDemo(nullptr)
#ifdef PLATFORM_WINDOWS
                          , window(nullptr)
#endif
{
    LOGI("GLRenderer constructor called");
    
    // Initialize clear color (default: dark teal)
    clearColor[0] = 0.2f;
    clearColor[1] = 0.3f;
    clearColor[2] = 0.3f;
}

GLRenderer::~GLRenderer() {
    LOGI("GLRenderer destructor called");
    cleanup();
}

bool GLRenderer::initialize(void* assetManager, const std::string& shaderDirectory) {
    LOGI("Initializing OpenGL ES renderer");
    
#ifdef PLATFORM_WINDOWS
    if (!initializeGLFW()) {
        LOGE("Failed to initialize GLFW");
        return false;
    }
#endif
    
    if (!initializeShaderManager(assetManager, shaderDirectory)) {
        LOGE("Failed to initialize shader manager");
        return false;
    }
    
    if (!createShaders()) {
        LOGE("Failed to create shaders");
        return false;
    }

    if (!createBuffers()) {
        LOGE("Failed to create buffers");
        return false;
    }

    if (!initializeImGui()) {
        LOGE("Failed to initialize ImGui");
        return false;
    }

    // Initialize audio system
    initializeAudio(assetManager);

    // Initialize reflection demo
    initializeReflectionDemo();

    LOGI("OpenGL ES renderer initialized successfully");
    return true;
}

#ifdef PLATFORM_WINDOWS
bool GLRenderer::initializeGLFW() {
    LOGI("Initializing GLFW");
    
    // Initialize GLFW
    if (!glfwInit()) {
        LOGE("Failed to initialize GLFW");
        return false;
    }
    
    // Configure GLFW for OpenGL ES
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    // Create window
    window = glfwCreateWindow(800, 600, "OpenGL ES 3.0 Triangle Demo", nullptr, nullptr);
    if (!window) {
        LOGE("Failed to create GLFW window");
        glfwTerminate();
        return false;
    }
    
    glfwMakeContextCurrent(window);
    
    // Initialize GLEW
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        LOGE("Failed to initialize GLEW");
        return false;
    }
    
    LOGI("GLFW initialized successfully");
    return true;
}

void GLRenderer::setWindow(GLFWwindow* win) {
    window = win;
}
#endif

bool GLRenderer::initializeShaderManager(void* assetManager, const std::string& shaderDirectory) {
    LOGI("Initializing shader manager");
    shaderManager = std::make_unique<ShaderManager>();
    if (!shaderManager) {
        LOGE("Failed to create shader manager");
        return false;
    }
    
    if (!shaderManager->initialize(assetManager, shaderDirectory)) {
        LOGE("Failed to initialize shader manager: %s", shaderManager->getLastError().c_str());
        return false;
    }
    
    LOGI("Shader manager initialized successfully");
    return true;
}

bool GLRenderer::createShaders() {
    LOGI("Creating shaders from files");
    
    if (!shaderManager) {
        LOGE("Shader manager not initialized");
        return false;
    }
    
    // Determine OpenGL version
    const char* version = (const char*)glGetString(GL_VERSION);
    LOGI("OpenGL version: %s", version ? version : "unknown");
    
    bool useOpenGLES3 = false;
    if (version && strstr(version, "OpenGL ES 3")) {
        useOpenGLES3 = true;
        LOGI("Using OpenGL ES 3.0 shaders");
    } else {
        LOGI("Using OpenGL ES 2.0 shaders");
    }
    
    // Choose appropriate shader file names
    std::string vertexFile, fragmentFile;
    if (useOpenGLES3) {
        vertexFile = "shaders/basic_vertex.glsl";
        fragmentFile = "shaders/basic_fragment.glsl";
    } else {
        vertexFile = "shaders/basic_vertex_es2.glsl";
        fragmentFile = "shaders/basic_fragment_es2.glsl";
    }
    
    // Create shader program from files
    program = shaderManager->createProgramFromFiles(vertexFile, fragmentFile);
    if (program == 0) {
        LOGE("Failed to create shader program from files: %s", 
             shaderManager->getLastError().c_str());
        return false;
    }
    
    // Store shader IDs for cleanup (they're now part of the program)
    vertexShader = 0; // Individual shaders are deleted after linking
    fragmentShader = 0;
    
    LOGI("Shader program created successfully from files");
    return true;
}

bool GLRenderer::createBuffers() {
    LOGI("Creating buffers");
    
    // Triangle vertices (position + color)
    float vertices[] = {
        // Position (x, y, z)    // Color (r, g, b)
         0.0f,  0.5f, 0.0f,     1.0f, 0.0f, 0.0f,  // Top vertex (red)
        -0.5f, -0.5f, 0.0f,     0.0f, 1.0f, 0.0f,  // Bottom left (green)
         0.5f, -0.5f, 0.0f,     0.0f, 0.0f, 1.0f   // Bottom right (blue)
    };

    // Check if VAOs are supported (OpenGL ES 3.0+)
    const char* version = (const char*)glGetString(GL_VERSION);
    bool useVAO = (version && strstr(version, "OpenGL ES 3"));
    
    if (useVAO) {
        // Create VAO (OpenGL ES 3.0+)
        glGenVertexArrays(1, &vao);
        if (vao == 0) {
            LOGE("Failed to create VAO");
            return false;
        }
        glBindVertexArray(vao);
        LOGI("Using VAO for vertex attributes");
    } else {
        LOGI("VAO not supported, using direct attribute binding");
    }

    // Create VBO
    glGenBuffers(1, &vbo);
    if (vbo == 0) {
        LOGE("Failed to create VBO");
        return false;
    }
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    if (useVAO) {
        // Position attribute
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // Color attribute
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        // Unbind
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    } else {
        // For OpenGL ES 2.0, we'll bind attributes in render function
        LOGI("Will bind attributes in render function for OpenGL ES 2.0");
    }

    LOGI("Buffers created successfully");
    return true;
}

void GLRenderer::render() {
    // Update audio system
    updateAudio();
    
    // Clear the screen
    glClearColor(clearColor[0], clearColor[1], clearColor[2], 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Use shader program
    glUseProgram(program);

    // Check if VAOs are supported
    const char* version = (const char*)glGetString(GL_VERSION);
    bool useVAO = (version && strstr(version, "OpenGL ES 3"));
    
    if (useVAO) {
        // Bind VAO and draw (OpenGL ES 3.0+)
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);
    } else {
        // For OpenGL ES 2.0, bind attributes manually
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        
        // Position attribute
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // Color attribute
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }

    // Render ImGui
    if (imguiInitialized) {
        // Try to render ImGui, and if it fails, try to recreate device objects
        static bool deviceObjectsCreated = false;
        if (!deviceObjectsCreated) {
            if (ImGui_ImplOpenGL3_CreateDeviceObjects()) {
                deviceObjectsCreated = true;
                LOGI("ImGui device objects created successfully");
            } else {
                LOGE("Failed to create ImGui device objects");
            }
        }
        
        if (deviceObjectsCreated) {
            renderImGui();
        }
    }

#ifdef PLATFORM_WINDOWS
    // Swap buffers and poll events for Windows
    if (window) {
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
#endif
}

void GLRenderer::cleanup() {
    LOGI("Cleaning up OpenGL resources");
    
    // Clean up audio system
    cleanupAudio();
    
    // Clean up reflection demo
    if (reflectionDemo) {
        reflectionDemo->cleanup();
        reflectionDemo.reset();
    }
    
    // Clean up ImGui
    cleanupImGui();
    
    // Clean up shader manager
    if (shaderManager) {
        shaderManager->cleanup();
        shaderManager.reset();
    }
    
    // Clean up OpenGL resources
    if (program != 0) {
        glDeleteProgram(program);
        program = 0;
    }
    if (vertexShader != 0) {
        glDeleteShader(vertexShader);
        vertexShader = 0;
    }
    if (fragmentShader != 0) {
        glDeleteShader(fragmentShader);
        fragmentShader = 0;
    }
    if (vbo != 0) {
        glDeleteBuffers(1, &vbo);
        vbo = 0;
    }
    if (vao != 0) {
        glDeleteVertexArrays(1, &vao);
        vao = 0;
    }
    
#ifdef PLATFORM_WINDOWS
    if (window) {
        glfwDestroyWindow(window);
        window = nullptr;
    }
    glfwTerminate();
#endif

    LOGI("OpenGL resources cleaned up");
}

bool GLRenderer::initializeImGui() {
    LOGI("Initializing ImGui");
    
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

#ifdef PLATFORM_WINDOWS
    // Setup Platform/Renderer backends
    // For Windows, we're using OpenGL ES 3.0, so we need GLSL ES 300
    const char* glsl_version = "#version 300 es";
    if (!ImGui_ImplGlfw_InitForOpenGL(window, true)) {
        LOGE("Failed to initialize ImGui GLFW backend");
        return false;
    }
    if (!ImGui_ImplOpenGL3_Init(glsl_version)) {
        LOGE("Failed to initialize ImGui OpenGL3 backend");
        return false;
    }
#elif defined(PLATFORM_ANDROID)
    // For Android, we need to get the display size
    // This will be handled by the Android activity
    const char* glsl_version = "#version 300 es";
    if (!ImGui_ImplOpenGL3_Init(glsl_version)) {
        LOGE("Failed to initialize ImGui OpenGL3 backend");
        return false;
    }
#endif

    // Set initial display size
    io.DisplaySize = ImVec2(800.0f, 600.0f);
    io.DeltaTime = 1.0f / 60.0f; // Initial delta time

#ifdef PLATFORM_ANDROID
    // Apply DPI scaling for Android
    AndroidImGuiHelper::applyDPIScaling();
#endif

    imguiInitialized = true;
    LOGI("ImGui initialized successfully");
    return true;
}

void GLRenderer::cleanupImGui() {
    if (imguiInitialized) {
        LOGI("Cleaning up ImGui");
        
#ifdef PLATFORM_WINDOWS
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
#elif defined(PLATFORM_ANDROID)
        ImGui_ImplOpenGL3_Shutdown();
#endif
        ImGui::DestroyContext();
        
        imguiInitialized = false;
        LOGI("ImGui cleaned up");
    }
}

void GLRenderer::renderImGui() {
    if (!imguiInitialized) return;

    try {
        // Start the Dear ImGui frame
#ifdef PLATFORM_WINDOWS
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
#elif defined(PLATFORM_ANDROID)
        AndroidImGuiHelper::updateImGuiIO();
        ImGui_ImplOpenGL3_NewFrame();
#endif
        ImGui::NewFrame();

        // Show demo window
        if (showDemoWindow) {
            ImGui::ShowDemoWindow(&showDemoWindow);
        }

        // Create a simple control window
        {
            ImGui::Begin("OpenGL ES Triangle Demo");
            ImGui::Text("Hello from ImGui!");
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 
                       1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            
#ifdef PLATFORM_ANDROID
            ImGui::Text("Device DPI: %.1f", AndroidImGuiHelper::getDPI());
            ImGui::Text("UI Scale: %.2fx", ImGui::GetIO().FontGlobalScale);
#endif
            
            if (ImGui::Button("Toggle Demo Window")) {
                showDemoWindow = !showDemoWindow;
            }
            
            ImGui::ColorEdit3("Clear Color", clearColor);
            
            // Audio controls
            ImGui::Separator();
            ImGui::Text("Audio Controls");
            if (audioManager) {
                static int frameCounter = 0;
                frameCounter++;
                
                // Add unique IDs to buttons to prevent ImGui confusion
                ImGui::PushID("victory_btn");
                if (ImGui::Button("Play Victory Sound")) {
                    LOGI("=== VICTORY BUTTON CLICKED! Frame: %d ===", frameCounter);
                    audioManager->playSound("victory", 0.5f);
                }
                ImGui::PopID();
                
                ImGui::PushID("ui_click_btn");
                if (ImGui::Button("Play UI Click")) {
                    LOGI("=== UI CLICK BUTTON CLICKED! Frame: %d ===", frameCounter);
                    audioManager->playSound("ui_click", 0.3f);
                }
                ImGui::PopID();
                
                ImGui::PushID("walking_btn");
                if (ImGui::Button("Play Walking Sound")) {
                    LOGI("=== WALKING BUTTON CLICKED! Frame: %d ===", frameCounter);
                    audioManager->playSound("walking", 0.4f);
                }
                ImGui::PopID();
                
                ImGui::PushID("stop_all_btn");
                if (ImGui::Button("Stop All Sounds")) {
                    LOGI("=== STOP ALL BUTTON CLICKED! Frame: %d ===", frameCounter);
                    audioManager->stopAllSounds();
                }
                ImGui::PopID();
                
                // Master volume control
                static float masterVolume = 1.0f;
                if (ImGui::SliderFloat("Master Volume", &masterVolume, 0.0f, 1.0f)) {
                    audioManager->setMasterVolume(masterVolume);
                }
            } else {
                ImGui::Text("Audio system not available");
            }
            
            ImGui::End();
        }

        // Render reflection demo within ImGui frame
        if (reflectionDemo) {
            try {
                reflectionDemo->render();
            } catch (const std::exception& e) {
                LOGE("Error in reflection demo render: %s", e.what());
            } catch (...) {
                LOGE("Unknown error in reflection demo render");
            }
        }

        // Rendering
        ImGui::Render();
        ImDrawData* draw_data = ImGui::GetDrawData();
        if (draw_data) {
            ImGui_ImplOpenGL3_RenderDrawData(draw_data);
        }
    } catch (const std::exception& e) {
        LOGE("Error in renderImGui: %s", e.what());
    } catch (...) {
        LOGE("Unknown error in renderImGui");
    }
}

void GLRenderer::showImGuiDemo() {
    showDemoWindow = true;
}

bool GLRenderer::recreateImGuiDeviceObjects() {
    if (!imguiInitialized) {
        LOGE("ImGui not initialized, cannot recreate device objects");
        return false;
    }
    
    LOGI("Recreating ImGui device objects");
    
    // Destroy existing device objects
    ImGui_ImplOpenGL3_DestroyDeviceObjects();
    
    // Recreate device objects
    bool success = ImGui_ImplOpenGL3_CreateDeviceObjects();
    if (success) {
        LOGI("ImGui device objects recreated successfully");
    } else {
        LOGE("Failed to recreate ImGui device objects");
    }
    
    return success;
}

#ifdef PLATFORM_ANDROID
void GLRenderer::setDisplaySize(int width, int height) {
    AndroidImGuiHelper::setDisplaySize(width, height);
}

void GLRenderer::handleTouchEvent(int action, float x, float y) {
    AndroidImGuiHelper::handleTouchEvent(action, x, y);
}

void GLRenderer::handleKeyEvent(int key, int action) {
    AndroidImGuiHelper::handleKeyEvent(key, action);
}

void GLRenderer::setDPI(float dpi) {
    AndroidImGuiHelper::setDPI(dpi);
}
#endif

// Audio system methods
void GLRenderer::initializeAudio(void* assetManager) {
    LOGI("Initializing audio system");
    
    audioManager = std::make_unique<AudioManager>();
    if (!audioManager) {
        LOGE("Failed to create audio manager");
        return;
    }
    
    if (!audioManager->initialize(assetManager)) {
        LOGE("Failed to initialize audio system");
        audioManager.reset();
        return;
    }
    
    // Load sample sounds
    loadSampleSounds();
    
    LOGI("Audio system initialized successfully");
}

void GLRenderer::updateAudio() {
    if (audioManager) {
        audioManager->update();
    }
}

void GLRenderer::cleanupAudio() {
    if (audioManager) {
        audioManager->shutdown();
        audioManager.reset();
        LOGI("Audio system cleaned up");
    }
}

void GLRenderer::loadSampleSounds() {
    if (!audioManager) {
        LOGI("AudioManager is null, cannot load sounds");
        return;
    }
    
    LOGI("Starting to load sample sounds...");
    
    // Load some sample sounds from the game assets
    bool victory = audioManager->loadSound("victory", "level-win.mp3");
    bool ui_click = audioManager->loadSound("ui_click", "mouse-click.mp3");
    bool walking = audioManager->loadSound("walking", "footsteps-male.mp3");
    bool lose = audioManager->loadSound("lose", "losing-horn.mp3");
    
    LOGI("Sound loading results: victory=%d, ui_click=%d, walking=%d, lose=%d", 
         victory, ui_click, walking, lose);
    
    // Check how many sounds are actually loaded
    auto loadedSounds = audioManager->getLoadedSounds();
    LOGI("Total sounds loaded: %zu", loadedSounds.size());
    for (const auto& soundName : loadedSounds) {
        LOGI("Loaded sound: %s", soundName.c_str());
    }
}

// Reflection demo methods
void GLRenderer::initializeReflectionDemo() {
    LOGI("Initializing reflection demo");
    
    reflectionDemo = std::make_unique<ReflectionDemo>();
    if (!reflectionDemo) {
        LOGE("Failed to create reflection demo");
        return;
    }
    
    reflectionDemo->initialize();
    LOGI("Reflection demo initialized successfully");
}

void GLRenderer::renderReflectionDemo() {
    if (reflectionDemo) {
        reflectionDemo->render();
    }
} 