#include "AndroidImGuiHelper.h"

// Define LOG_TAG for this file
#define LOG_TAG "AndroidImGuiHelper"

#ifdef PLATFORM_ANDROID
#include <chrono>

// Static member definitions
int AndroidImGuiHelper::displayWidth = 800;
int AndroidImGuiHelper::displayHeight = 600;
bool AndroidImGuiHelper::touchDown = false;
float AndroidImGuiHelper::lastTouchX = 0.0f;
float AndroidImGuiHelper::lastTouchY = 0.0f;
float AndroidImGuiHelper::deviceDPI = 160.0f; // Default to mdpi (160dpi)
bool AndroidImGuiHelper::dpiScalingApplied = false;

void AndroidImGuiHelper::setDisplaySize(int width, int height) {
    LOGI("Setting display size: %dx%d", width, height);
    displayWidth = width;
    displayHeight = height;
    
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)width, (float)height);
}

void AndroidImGuiHelper::handleTouchEvent(int action, float x, float y) {
    ImGuiIO& io = ImGui::GetIO();
    
    // Convert screen coordinates to ImGui coordinates
    float imguiX = x;
    float imguiY = y;
    
    switch (action) {
        case 0: // ACTION_DOWN
            touchDown = true;
            lastTouchX = imguiX;
            lastTouchY = imguiY;
            io.MouseDown[0] = true;
            io.MousePos = ImVec2(imguiX, imguiY);
            LOGI("Touch DOWN at (%.2f, %.2f)", imguiX, imguiY);
            break;
            
        case 1: // ACTION_UP
            touchDown = false;
            io.MouseDown[0] = false;
            LOGI("Touch UP at (%.2f, %.2f)", imguiX, imguiY);
            break;
            
        case 2: // ACTION_MOVE
            if (touchDown) {
                io.MousePos = ImVec2(imguiX, imguiY);
            }
            break;
    }
}

void AndroidImGuiHelper::handleKeyEvent(int key, int action) {
    ImGuiIO& io = ImGui::GetIO();
    
    // Map Android key codes to ImGui keys
    ImGuiKey imguiKey = ImGuiKey_None;
    switch (key) {
        case 4: // KEYCODE_BACK
            imguiKey = ImGuiKey_Escape;
            break;
        case 66: // KEYCODE_ENTER
            imguiKey = ImGuiKey_Enter;
            break;
        case 67: // KEYCODE_DEL
            imguiKey = ImGuiKey_Backspace;
            break;
        // Add more key mappings as needed
    }
    
    if (imguiKey != ImGuiKey_None) {
        io.AddKeyEvent(imguiKey, action == 0); // 0 = down, 1 = up
    }
}

void AndroidImGuiHelper::updateImGuiIO() {
    ImGuiIO& io = ImGui::GetIO();
    
    // Update display size
    io.DisplaySize = ImVec2((float)displayWidth, (float)displayHeight);
    
    // Set delta time (this should be called with actual frame time)
    static auto lastTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    auto deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
    io.DeltaTime = deltaTime;
    lastTime = currentTime;
    
    // Clear mouse wheel delta (important for proper input handling)
    io.MouseWheel = 0.0f;
    io.MouseWheelH = 0.0f;
    
    // Force clear any lingering mouse button states
    if (!touchDown) {
        io.MouseDown[0] = false;
        io.MouseDown[1] = false;
        io.MouseDown[2] = false;
    }
}

void AndroidImGuiHelper::setDPI(float dpi) {
    LOGI("Setting device DPI: %.1f", dpi);
    deviceDPI = dpi;
    dpiScalingApplied = false; // Reset scaling flag so it can be reapplied
}

void AndroidImGuiHelper::applyDPIScaling() {
    if (dpiScalingApplied) {
        return; // Already applied
    }
    
    ImGuiIO& io = ImGui::GetIO();
    ImGuiStyle& style = ImGui::GetStyle();
    
    // Calculate scale factor based on device DPI
    // 160dpi is the baseline (mdpi) on Android
    //float scaleFactor = deviceDPI / 160.0f;
    float scaleFactor = 3.0f;

    // Ensure minimum scaling for very small screens
    // Ensure minimum scaling for very small screens
    scaleFactor = std::max(scaleFactor, 1.2f);
    
    LOGI("Applying DPI scaling: DPI=%.1f, Scale=%.2f", deviceDPI, scaleFactor);
    
    // Log current style values before scaling
    LOGI("Style before scaling - WindowPadding: %.1f, FramePadding: %.1f, ItemSpacing: %.1f", 
         style.WindowPadding.x, style.FramePadding.x, style.ItemSpacing.x);
    
    // Apply scaling to style (widgets, padding, spacing)
    style.ScaleAllSizes(scaleFactor);
    
    // Apply scaling to fonts
    io.FontGlobalScale = scaleFactor;
    
    // Log style values after scaling
    LOGI("Style after scaling - WindowPadding: %.1f, FramePadding: %.1f, ItemSpacing: %.1f", 
         style.WindowPadding.x, style.FramePadding.x, style.ItemSpacing.x);
    
    // Mark as applied
    dpiScalingApplied = true;
    
    LOGI("DPI scaling applied successfully - FontGlobalScale: %.2f", io.FontGlobalScale);
}

#else
// Stub implementations for non-Android platforms

// Static member definitions
int AndroidImGuiHelper::displayWidth = 800;
int AndroidImGuiHelper::displayHeight = 600;
bool AndroidImGuiHelper::touchDown = false;
float AndroidImGuiHelper::lastTouchX = 0.0f;
float AndroidImGuiHelper::lastTouchY = 0.0f;
float AndroidImGuiHelper::deviceDPI = 160.0f; // Default to mdpi (160dpi)
bool AndroidImGuiHelper::dpiScalingApplied = false;

void AndroidImGuiHelper::setDisplaySize(int width, int height) {
    // Stub implementation - do nothing on non-Android platforms
    (void)width;
    (void)height;
}

void AndroidImGuiHelper::handleTouchEvent(int action, float x, float y) {
    // Stub implementation - do nothing on non-Android platforms
    (void)action;
    (void)x;
    (void)y;
}

void AndroidImGuiHelper::handleKeyEvent(int key, int action) {
    // Stub implementation - do nothing on non-Android platforms
    (void)key;
    (void)action;
}

void AndroidImGuiHelper::updateImGuiIO() {
    // Stub implementation - do nothing on non-Android platforms
}

void AndroidImGuiHelper::setDPI(float dpi) {
    // Stub implementation - do nothing on non-Android platforms
    (void)dpi;
}

void AndroidImGuiHelper::applyDPIScaling() {
    // Stub implementation - do nothing on non-Android platforms
}

#endif // PLATFORM_ANDROID
