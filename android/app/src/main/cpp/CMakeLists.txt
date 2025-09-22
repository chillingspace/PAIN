cmake_minimum_required(VERSION 3.22)
project(pain_android_entry LANGUAGES C CXX)

if(ANDROID)
    # Build the entry shared lib that your NativeActivity loads
    add_library(pain SHARED
            android_entry.cpp
            native_app_glue/android_native_app_glue.c   # vendored glue
    )

    # Include path for the glue header
    target_include_directories(pain PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/native_app_glue
    )

    find_library(log-lib log)
    find_library(android-lib android)
    find_library(egl-lib EGL)
    find_library(gles-lib GLESv3)

    # Link against your engine + game core and Android libs
    target_link_libraries(pain
            PRIVATE
            GameCore         # defined by your Game/CMake on ANDROID
            PAINEngine       # defined by your Engine/CMake
            android
            log
            EGL
            GLESv3
    )

    target_compile_definitions(pain PRIVATE IMGUI_IMPL_OPENGL_ES3=1)
endif()