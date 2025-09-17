// PAINEngine/src/PAINEngine/Platform.h

#ifndef PLATFORM_H
#define PLATFORM_H

// --- Platform Detection ---
#ifdef _WIN32
    #define PN_PLATFORM_WINDOWS
#elif defined(__ANDROID__)
    #define PN_PLATFORM_ANDROID
#else
    #error "Unknown platform!"
#endif

// --- Platform-specific Includes ---
#ifdef PN_PLATFORM_WINDOWS
    // Includes for Windows
    #include <Windows.h>
    #include <GL/glew.h>
    #include <GLFW/glfw3.h>
#elif defined(PN_PLATFORM_ANDROID)
    // Includes for Android
    #include <GLES3/gl3.h>
    #include <android/log.h>
#endif

// --- ImGui Backend Includes ---
// The main "imgui.h" is in your pch.h, so we only need the platform-specific backends here.
#ifdef PN_PLATFORM_WINDOWS
    #include "ImGui/headers/imgui_impl_glfw.h"
    #include "ImGui/headers/imgui_impl_opengl3.h"
#elif defined(PN_PLATFORM_ANDROID)
    #include "ImGui/headers/imgui_impl_opengl3.h"
#endif

#endif // PLATFORM_H