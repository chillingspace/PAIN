# android/app/src/main/cpp/CMakeLists.txt

cmake_minimum_required(VERSION 3.16)

project(PAINEngineJNI) # The name of our native library

# Ensure we're building for Android
if(NOT ANDROID)
    message(FATAL_ERROR "This CMakeLists.txt is intended for Android builds only.")
endif()

# --- Add Global Definitions for C++ Code ---
add_definitions(-DPLATFORM_ANDROID)

# --- Define Path to Shared Engine Source Code ---
# This path is relative from this file's location to your PAINEngine's source.
set(ENGINE_SRC_DIR ../../../../../PAINEngine/src)
file(GLOB_RECURSE ENGINE_SOURCES "${ENGINE_SRC_DIR}/*.cpp" "${ENGINE_SRC_DIR}/*.h")

# --- FMOD Configuration ---
# Set the include directory for FMOD headers
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../../vendor/FMOD/android/api/core/inc)

# Add the FMOD library path. ${ANDROID_ABI} ensures it picks the right one (arm64-v8a, etc.)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../../vendor/FMOD/android/api/core/lib/${ANDROID_ABI})

# --- Add Native Library Target ---
add_library(${PROJECT_NAME} SHARED
    # Add your JNI bridge file here (we will create this later)
    JNIBridge.cpp 
    
    # Add all the engine source files
    ${ENGINE_SOURCES}
)

# --- Link System and FMOD Libraries ---
target_link_libraries(${PROJECT_NAME}
    # Standard Android libraries
    android
    log
    GLESv3
    EGL

    # Link the FMOD libraries by name (CMake will find them in the link_directories path)
    fmod
    fmodL
)