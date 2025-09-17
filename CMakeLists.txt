# ===================================================================
# PAIN Engine - Root CMakeLists.txt (Corrected)
# ===================================================================
cmake_minimum_required(VERSION 3.16)
project(PAIN)

# --- Project-wide Settings ---
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- MSVC Runtime Library (CRITICAL for linking) ---
# This is the definitive fix. It forces all targets to use the static
# runtime library (/MT for Release, /MTd for Debug), which matches
# the pre-compiled libraries like GLFW and GLEW, resolving the LNK errors.
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# --- Custom CMake Modules ---
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(ImportDependencies)

# --- Platform Detection ---
if(CMAKE_SYSTEM_NAME STREQUAL "Android")
    set(ANDROID TRUE)
endif()

# --- Output Directories (for Desktop builds) ---
if(NOT ANDROID)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/${CMAKE_BUILD_TYPE})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/${CMAKE_BUILD_TYPE})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/${CMAKE_BUILD_TYPE})
endif()

# --- Add Engine and Game Subprojects ---
add_subdirectory(PAINEngine)
add_subdirectory(Game)

# --- Visual Studio Specific Properties ---
if(NOT ANDROID)
    set_property(DIRECTORY . PROPERTY VS_STARTUP_PROJECT Game)
endif()