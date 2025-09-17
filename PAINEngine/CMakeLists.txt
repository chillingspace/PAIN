cmake_minimum_required(VERSION 3.25)
project(PAINEngine)

# -------------------------
# Collect Common (Cross-Platform) Engine Sources
# -------------------------
set(COMMON_ENGINE_SRC
    "src/PAINEngine/pch.cpp"
    "src/PAINEngine/Applications/Application.cpp"
    "src/PAINEngine/Applications/AppSystem.cpp"
    "src/PAINEngine/Audio/AudioManager.cpp"
    "src/PAINEngine/CoreSystems/Collision/sCollision.cpp"
    "src/PAINEngine/CoreSystems/Collision/sLayer.cpp"
    "src/PAINEngine/CoreSystems/Renderer/Shader.cpp"
    "src/PAINEngine/CoreSystems/Renderer/TestTriangleLayer.cpp"
    "src/PAINEngine/ECS/Controller.cpp"
    "src/PAINEngine/ECS/System/System.cpp"
    "src/PAINEngine/LayeredSystems/LevelEditor/Command.cpp"
    "src/PAINEngine/LayeredSystems/LevelEditor/Editor.cpp"
    "src/PAINEngine/LayeredSystems/LevelEditor/Panels/Panels.cpp"
    "src/PAINEngine/LayeredSystems/LevelEditor/Panels/ToolsPanel.cpp"
    "src/PAINEngine/Logging/Log.cpp"
    "src/PAINEngine/Systems/Physics/sysPhysics.cpp"
)

# -------------------------
# Collect Desktop-Only Engine Sources
# -------------------------
set(DESKTOP_ONLY_ENGINE_SRC
    "src/PAINEngine/CoreSystems/Windows/GLFW/GLFWWindow.cpp"
    "src/PAINEngine/CoreSystems/Windows/OpenGL/OpenGLContext.cpp"
    "src/PAINEngine/CoreSystems/Windows/OpenGL/ImGuiOpenGLRenderer.cpp"
)

# -------------------------
# Define ImGui Sources
# -------------------------
set(IMGUI_SOURCES
    "${CMAKE_SOURCE_DIR}/PAINEngine/vendor/ImGui/src/imgui.cpp"
    "${CMAKE_SOURCE_DIR}/PAINEngine/vendor/ImGui/src/imgui_demo.cpp"
    "${CMAKE_SOURCE_DIR}/PAINEngine/vendor/ImGui/src/imgui_draw.cpp"
    "${CMAKE_SOURCE_DIR}/PAINEngine/vendor/ImGui/src/imgui_tables.cpp"
    "${CMAKE_SOURCE_DIR}/PAINEngine/vendor/ImGui/src/imgui_widgets.cpp"
    "${CMAKE_SOURCE_DIR}/PAINEngine/vendor/ImGui/src/imgui_impl_opengl3.cpp"
)
if(NOT ANDROID)
    list(APPEND IMGUI_SOURCES
        "${CMAKE_SOURCE_DIR}/PAINEngine/vendor/ImGui/src/imgui_impl_glfw.cpp"
    )
endif()

# -------------------------
# Build ImGui Library
# -------------------------
add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC "${CMAKE_SOURCE_DIR}/PAINEngine/vendor")
target_compile_definitions(imgui PUBLIC IMGUI_DEFINE_MATH_OPERATORS) # Ensures math operators are enabled
if(NOT ANDROID)
    target_compile_definitions(imgui PUBLIC IMGUI_IMPL_OPENGL_LOADER_GLEW)
endif()

# -------------------------
# Build Engine Library
# -------------------------
add_library(PAINEngine STATIC ${COMMON_ENGINE_SRC})
if(NOT ANDROID)
    target_sources(PAINEngine PRIVATE ${DESKTOP_ONLY_ENGINE_SRC})
endif()

# -------------------------
# Precompiled Header
# -------------------------
target_precompile_headers(PAINEngine
    PRIVATE "src/PAINEngine/pch.h"
)

# -------------------------
# Include Directories
# -------------------------
target_include_directories(PAINEngine PUBLIC
    "src"
    "src/PAINEngine"
    SYSTEM "vendor/spdlog/include"
    SYSTEM "vendor/glm"
    SYSTEM "vendor/Jolt"
)
if(NOT ANDROID)
    target_include_directories(PAINEngine PUBLIC
        SYSTEM "vendor/GLFW/include"
        SYSTEM "vendor/GL"
    )
endif()

# -------------------------
# Link Libraries & Definitions (Desktop Only)
# -------------------------
if(NOT ANDROID)
    target_link_libraries(PAINEngine PUBLIC
        imgui
        "${CMAKE_SOURCE_DIR}/PAINEngine/dependencies/lib/glfw3.lib"
        "${CMAKE_SOURCE_DIR}/PAINEngine/dependencies/lib/glew32s.lib"
        opengl32
    )
    target_compile_definitions(PAINEngine PUBLIC PN_PLATFORM_WINDOWS GLEW_STATIC)
endif()