cmake_minimum_required(VERSION 3.16)
project(Game)

# --- Global Definitions ---
add_definitions(-DPLATFORM_ANDROID)
add_definitions(-DIMGUI_IMPL_OPENGL_ES3)

# --- Define Paths ---
set(ENGINE_SRC_DIR ../../../../../PAINEngine/src)
set(ENGINE_VENDOR_DIR ../../../../../PAINEngine/vendor)
set(ROOT_VENDOR_DIR ../../../../../vendor)

# --- Collect Source Files (No Physics/ECS) ---
set(ENGINE_SOURCES
    "${ENGINE_SRC_DIR}/PAINEngine/pch.cpp"
    "${ENGINE_SRC_DIR}/PAINEngine/Applications/Application.cpp"
    "${ENGINE_SRC_DIR}/PAINEngine/Applications/AppSystem.cpp"
    "${ENGINE_SRC_DIR}/PAINEngine/Audio/AudioManager.cpp"
    "${ENGINE_SRC_DIR}/PAINEngine/CoreSystems/Renderer/Shader.cpp"
    "${ENGINE_SRC_DIR}/PAINEngine/CoreSystems/Renderer/TestTriangleLayer.cpp"
    "${ENGINE_SRC_DIR}/PAINEngine/Logging/Log.cpp"
)

set(IMGUI_SOURCES
    "${ENGINE_VENDOR_DIR}/ImGui/src/imgui.cpp"
    "${ENGINE_VENDOR_DIR}/ImGui/src/imgui_demo.cpp"
    "${ENGINE_VENDOR_DIR}/ImGui/src/imgui_draw.cpp"
    "${ENGINE_VENDOR_DIR}/ImGui/src/imgui_tables.cpp"
    "${ENGINE_VENDOR_DIR}/ImGui/src/imgui_widgets.cpp"
    "${ENGINE_VENDOR_DIR}/ImGui/src/imgui_impl_opengl3.cpp"
)

# --- Build the Native Library ---
add_library(Game SHARED
    JNIBridge.cpp
    ../../../../../Game/src/Game.cpp
    ${ENGINE_SOURCES}
    ${IMGUI_SOURCES}
)

# --- Set Include Directories ---
target_include_directories(Game PRIVATE
    # Path for pch.h, Application.h, etc.
    "${ENGINE_SRC_DIR}"
    # Path for ImGui, spdlog, glm
    "${ENGINE_VENDOR_DIR}"
    # Path for FMOD Core headers ONLY
    "${ROOT_VENDOR_DIR}/FMOD/android/api/core/inc"
)

# --- Link Libraries ---
find_library(log-lib log)
find_library(egl-lib EGL)
find_library(glesv3-lib GLESv3)

# Add the prebuilt FMOD Core library
add_library(fmod SHARED IMPORTED)
set_target_properties(fmod PROPERTIES IMPORTED_LOCATION
    "${ROOT_VENDOR_DIR}/FMOD/android/api/core/lib/${ANDROID_ABI}/libfmod.so")

# Link your Game library against FMOD and required Android libraries
target_link_libraries(Game fmod ${log-lib} ${egl-lib} ${glesv3-lib})

# --- Final Target Properties ---
set_target_properties(Game PROPERTIES OUTPUT_NAME "Game")

# --- Enable Precompiled Headers ---
target_precompile_headers(Game PRIVATE
    "${ENGINE_SRC_DIR}/PAINEngine/pch.h"
)