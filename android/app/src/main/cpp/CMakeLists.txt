# ======================= Android CMake =========================
cmake_minimum_required(VERSION 3.22.1)
project(PAINEngineJNI LANGUAGES C CXX)

# --- C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Safety check
if(NOT ANDROID)
    message(FATAL_ERROR "This CMakeLists.txt is intended for Android builds only.")
endif()

# --- Global definitions
add_definitions(-DANDROID)
add_definitions(-DPLATFORM_ANDROID)

# --- Engine Source
set(ENGINE_SRC_DIR ../../../../../PAINEngine/src)
file(GLOB_RECURSE ENGINE_SOURCES
    "${ENGINE_SRC_DIR}/*.cpp"
    "${ENGINE_SRC_DIR}/*.h"
)

# --- JNI Bridge + Engine Sources
add_library(${PROJECT_NAME} SHARED
    native-lib.cpp
    ${ENGINE_SOURCES}
)

# --- Include Paths
include_directories(${ENGINE_SRC_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_LIST_DIR}/../../../../../vendor/FMOD/android/api/core/inc)

# --- FMOD Libs
set(FMOD_LIB_DIR ${CMAKE_CURRENT_LIST_DIR}/../../../../../vendor/FMOD/android/api/core/lib/${ANDROID_ABI})

# --- Linking
target_link_libraries(${PROJECT_NAME}
    android
    log
    GLESv3
    EGL
    # FMOD shared libs
    ${FMOD_LIB_DIR}/libfmod.so
    ${FMOD_LIB_DIR}/libfmodL.so
)
