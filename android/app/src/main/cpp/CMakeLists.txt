# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html.
# For more examples on how to use CMake, see https://github.com/android/ndk-samples.

# Sets the minimum CMake version required for this project.
cmake_minimum_required(VERSION 3.22.1)

set(VCPKG_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../../../libs/vcpkg")

# Set the directory where custom CMake modules are stored
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/../../../../../cmake")
#if(CMAKE_SYSTEM_NAME STREQUAL "Android")
	set(ANDROID 1)
	set(VCPKG_TARGET_ANDROID 1)
	# this file must be included before everything else
	include("${CMAKE_CURRENT_LIST_DIR}/../../../../../cmake/vcpkg_android.cmake")
#endif()

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# FMOD is now enabled for Android

# Declares the project name. The project name can be accessed via ${ PROJECT_NAME},
# Since this is the top level CMakeLists.txt, the project name is also accessible
# with ${CMAKE_PROJECT_NAME} (both CMake variables are in-sync within the top level
# build script scope).
project("jnicpp")

# Ensure we're building for Android
if(NOT ANDROID)
    message(FATAL_ERROR "This CMakeLists.txt is intended for Android builds only")
endif()

# Add definitions
add_definitions(-DANDROID)
add_definitions(-DPLATFORM_ANDROID)

# Include project-wide utility and dependency modules
include(ImportDependencies)
importDependencies()

find_package(imgui CONFIG REQUIRED)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.
#
# In this top level CMakeLists.txt, ${CMAKE_PROJECT_NAME} is used to define
# the target library name; in the sub-module's CMakeLists.txt, ${PROJECT_NAME}
# is preferred for the same purpose.
#
# In order to load a library into your app from Java/Kotlin, you must call
# System.loadLibrary() and pass the name of the library defined here;
# for GameActivity/NativeActivity derived applications, the same library name must be
# used in the AndroidManifest.xml file.

set(SRC_DIR ../../../../../my_gl_app)

# Get source files from my_gl_app directory
file(GLOB_RECURSE MY_GL_APP_SRC
        "${SRC_DIR}/*.cpp"
        "${SRC_DIR}/*.h"
        "${SRC_DIR}/*.hpp"
)

# Include directories
include_directories(${SRC_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Include FMOD headers
include_directories(${CMAKE_CURRENT_LIST_DIR}/libs/fmod/inc)

# Add FMOD library path
link_directories(${CMAKE_CURRENT_LIST_DIR}/libs/fmod/lib/${ANDROID_ABI})

add_library(${CMAKE_PROJECT_NAME} SHARED
        # List C/C++ source files with relative paths to this CMakeLists.txt.
        NativeTemplate.cpp
        ${MY_GL_APP_SRC})

# Specifies libraries CMake should link to your target library. You
# can link libraries from various origins, such as libraries defined in this
# build script, prebuilt third-party libraries, or Android system libraries.
target_link_libraries(${CMAKE_PROJECT_NAME}
        # List libraries link to the target library
        android
        log
        GLESv3
        EGL
        imgui::imgui
        nlohmann_json::nlohmann_json
        refl-cpp::refl-cpp
        # Link FMOD libraries with full path
        ${CMAKE_CURRENT_LIST_DIR}/libs/fmod/lib/${ANDROID_ABI}/libfmod.so
        ${CMAKE_CURRENT_LIST_DIR}/libs/fmod/lib/${ANDROID_ABI}/libfmodL.so)