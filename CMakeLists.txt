cmake_minimum_required(VERSION 3.16)
project(PAIN)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Set Output Directories (for Windows) ---
if(NOT ANDROID)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/${CMAKE_BUILD_TYPE})
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/${CMAKE_BUILD_TYPE})
endif()

# --- Dependency Management ---
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(ImportDependencies)
importDependencies()

# --- Subdirectories ---
add_subdirectory(PAINEngine)

# Add the Game executable target ONLY for desktop builds
if(NOT ANDROID)
    add_subdirectory(Game)
    # Set the startup project for Visual Studio
    set_property(DIRECTORY . PROPERTY VS_STARTUP_PROJECT Game)
endif()