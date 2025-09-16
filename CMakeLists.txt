cmake_minimum_required(VERSION 3.16)
project(PAIN)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set the directory for our custom CMake modules
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Include our dependency helper module
include(ImportDependencies)
importDependencies() # Placeholder

# --- Platform Detection and Global Definitions ---
if(NOT ANDROID)
    # PC-specific definitions (e.g., for static GLEW if you use it)
    add_definitions(-DPLATFORM_WINDOWS) # Assuming Windows for PC
else()
    # Android-specific definitions
    add_definitions(-DPLATFORM_ANDROID)
endif()

# --- FMOD Configuration ---
if(ANDROID)
    # For Android, paths are handled by the android/app/src/main/cpp/CMakeLists.txt but define the library names for consistency
    set(FMOD_LIBS fmod fmodL)
else()
    # For PC, set include and library paths
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/vendor/FMOD/windows/api/core/inc)
    link_directories(${CMAKE_CURRENT_SOURCE_DIR}/vendor/FMOD/windows/api/core/lib/x64)
    set(FMOD_LIBS fmod_vc fmodL_vc)
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
