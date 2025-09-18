cmake_minimum_required(VERSION 3.21)

project(PAIN_Engine)

# ========================================================================================
# Boilerplate for vcpkg toolchain. This ensures libraries are found correctly.
# ========================================================================================
if(DEFINED CMAKE_TOOLCHAIN_FILE)
    include(${CMAKE_TOOLCHAIN_FILE})
elseif(DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
    include(${CMAKE_TOOLCHAIN_FILE})
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ========================================================================================
# Add Subdirectories for the Engine and the Game
# ========================================================================================
add_subdirectory(PAINEngine)
add_subdirectory(Game)

# ========================================================================================
# Android-Specific Configuration
# ========================================================================================
if(ANDROID)
    # The main Android library is our "Game" target.
    # We must name it 'libGame.so' for Android to load it correctly.
    set_target_properties(Game PROPERTIES OUTPUT_NAME "Game")

    # Link against Android's native libraries for graphics and logging.
    target_link_libraries(Game PRIVATE
        android
        log
        EGL
        GLESv3
    )
endif()

# ========================================================================================
# Windows-Specific Final Steps (for Visual Studio)
# ========================================================================================
if(NOT ANDROID)
    set_property(DIRECTORY . PROPERTY VS_STARTUP_PROJECT Game)
endif()