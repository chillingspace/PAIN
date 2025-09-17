cmake_minimum_required(VERSION 3.16)
project(Game)

# --- Add Global Definitions for C++ Code ---
add_definitions(-DPLATFORM_ANDROID)
add_definitions(-DIMGUI_IMPL_OPENGL_ES3)

# --- Define Paths to Shared Engine Source Code ---
set(ENGINE_SRC_DIR ../../../../../PAINEngine/src/PAINEngine)
set(IMGUI_VENDOR_DIR ../../../../../PAINEngine/vendor/ImGui)

# --- Manually list all required source files (NO JOLT/PHYSICS) ---
set(ENGINE_SOURCES
    "${ENGINE_SRC_DIR}/pch.cpp"
    "${ENGINE_SRC_DIR}/Applications/Application.cpp"
    "${ENGINE_SRC_DIR}/Applications/AppSystem.cpp"
    "${ENGINE_SRC_DIR}/Audio/AudioManager.cpp"
    "${ENGINE_SRC_DIR}/CoreSystems/Renderer/Shader.cpp"
    "${ENGINE_SRC_DIR}/CoreSystems/Renderer/TestTriangleLayer.cpp"
    "${ENGINE_SRC_DIR}/Logging/Log.cpp"
)

set(IMGUI_SOURCES
    "${IMGUI_VENDOR_DIR}/src/imgui.cpp"
    "${IMGUI_VENDOR_DIR}/src/imgui_demo.cpp"
    "${IMGUI_VENDOR_DIR}/src/imgui_draw.cpp"
    "${IMGUI_VENDOR_DIR}/src/imgui_tables.cpp"
    "${IMGUI_VENDOR_DIR}/src/imgui_widgets.cpp"
    "${IMGUI_VENDOR_DIR}/src/imgui_impl_opengl3.cpp"
)

# --- Add Native Library Target ---
add_library(${PROJECT_NAME} SHARED
    JNIBridge.cpp
    ../../../../../Game/src/Game.cpp
    ${ENGINE_SOURCES}
    ${IMGUI_SOURCES}
)

# --- Include Directories ---
target_include_directories(${PROJECT_NAME} PRIVATE
    # Add all necessary include paths relative to this CMakeLists.txt
    ../../../../../PAINEngine/src
    ../../../../../PAINEngine/vendor
    ../../../../../vendor/FMOD/android/api/core/inc
    ../../../../../vendor/FMOD/android/api/studio/inc
)

# --- Link Libraries ---
target_link_libraries(${PROJECT_NAME}
    android log EGL GLESv3
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../vendor/FMOD/android/api/core/lib/${ANDROID_ABI}/libfmod.so
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../vendor/FMOD/android/api/core/lib/${ANDROID_ABI}/libfmodL.so
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../vendor/FMOD/android/api/studio/lib/${ANDROID_ABI}/libfmodstudio.so
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../vendor/FMOD/android/api/studio/lib/${ANDROID_ABI}/libfmodstudioL.so
)

# --- Final Target Properties ---
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "Game")