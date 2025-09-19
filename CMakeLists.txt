# ======================= Project Configuration =========================
# Cmake version
cmake_minimum_required(VERSION 3.16)

# Main Project
project(PAIN)


# Set the CMake policy version (this enables all policies up to CMake 3.5)
cmake_policy(VERSION 3.5)

# Set Cmake Standards
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set the directory for our custom CMake modules
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Include our dependency helper module
include(ImportDependencies)
importDependencies() # Placeholder

# Global compile flags and macros
if(NOT ANDROID)
    add_definitions(-DGLEW_STATIC)
else()
    add_definitions(-DPLATFORM_ANDROID)
endif()

# ======================= Vendors  =========================

include(VendorTargets)   # defines imgui, glfw, glm, nlohmann_json, spdlog_header_only, Jolt::Jolt, etc.

# ======================= Project Dependencies  =========================

# List of external libraries (platform-specific)
if(ANDROID)
    # On Android, we link NDK GL + imgui (built above) + your existing libs.
    set(ALL_LIBS
        PRIVATE imgui::imgui
        PRIVATE ${EGL_LIB} ${GLES_LIB} ${ANDROID_LIB} ${LOG_LIB}
        PRIVATE fmod
        PRIVATE fmodL
        PRIVATE refl-cpp::refl-cpp
        PRIVATE nlohmann_json::nlohmann_json
    )
else()
    # On Windows, link OpenGL, GLFW, and GLEW (all vendored, no vcpkg).
    set(ALL_LIBS
        PRIVATE OpenGL::GL
        PRIVATE glfw
        PRIVATE libglew_static
        PRIVATE imgui::imgui
        PRIVATE fmod_vc
        PRIVATE refl-cpp::refl-cpp
        PRIVATE nlohmann_json::nlohmann_json
    )
endif()

# --- Output Directories ---
# Keep your bin/bin-int structure
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin-int/${CMAKE_BUILD_TYPE})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE})

# --- Add Subprojects ---
add_subdirectory(PAINEngine)
add_subdirectory(Game)

# Set the startup project for the Visual Studio solution
set_property(DIRECTORY . PROPERTY VS_STARTUP_PROJECT Game)