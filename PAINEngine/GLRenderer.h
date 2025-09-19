#ifndef GLRENDERER_H
#define GLRENDERER_H

#include "Platform.h"
#include "ShaderManager.h"
#include "AudioManager.h"
#include "ReflectionDemo.h"
#include <memory>

#ifdef PLATFORM_ANDROID
#include "AndroidImGuiHelper.h"
#endif

class GLRenderer {
public:
    GLRenderer();
    ~GLRenderer();

    bool initialize(void* assetManager = nullptr, const std::string& shaderDirectory = "");
    void render();
    void cleanup();

#ifdef PLATFORM_WINDOWS
    // Windows-specific methods
    bool initializeGLFW();
    void setWindow(GLFWwindow* window);
    GLFWwindow* getWindow() const { return window; }
#endif

    // ImGui methods
    void renderImGui();
    void showImGuiDemo();
    bool recreateImGuiDeviceObjects();
    
    // Reflection demo methods
    void initializeReflectionDemo();
    void renderReflectionDemo();
    
    // Audio methods
    void initializeAudio(void* assetManager = nullptr);
    void updateAudio();
    void cleanupAudio();
    void loadSampleSounds();
    
#ifdef PLATFORM_ANDROID
    // Android-specific ImGui methods
    void setDisplaySize(int width, int height);
    void handleTouchEvent(int action, float x, float y);
    void handleKeyEvent(int key, int action);
    void setDPI(float dpi);
#endif

private:
    bool createShaders();
    bool createBuffers();
    bool initializeImGui();
    void cleanupImGui();
    bool initializeShaderManager(void* assetManager, const std::string& shaderDirectory = "");

    // OpenGL variables
    GLuint program;
    GLuint vertexShader;
    GLuint fragmentShader;
    GLuint vbo;
    GLuint vao;
    
    // Shader management
    std::unique_ptr<ShaderManager> shaderManager;
    
    // Audio management
    std::unique_ptr<AudioManager> audioManager;
    
    // Reflection demo
    std::unique_ptr<ReflectionDemo> reflectionDemo;

#ifdef PLATFORM_WINDOWS
    GLFWwindow* window;
#endif

    // ImGui variables
    bool imguiInitialized;
    bool showDemoWindow;
    
    // Clear color variables
    float clearColor[3];
};

#endif // GLRENDERER_H 