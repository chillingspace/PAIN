cmake_minimum_required(VERSION 3.25)
project(PAINEngine)

# -------------------------
# Collect engine sources
# -------------------------
file(GLOB_RECURSE ENGINE_SRC
    "${CMAKE_CURRENT_SOURCE_DIR}/src/PAINEngine/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/PAINEngine/*.c"
)

# -------------------------
# Build engine as static lib
# -------------------------
add_library(PAINEngine STATIC ${ENGINE_SRC})

# -------------------------
# Precompiled header
# -------------------------
target_precompile_headers(PAINEngine
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/PAINEngine/pch.h"
)

# -------------------------
# Include directories
# -------------------------
target_include_directories(PAINEngine PUBLIC
    ${CMAKE_SOURCE_DIR}/PAINEngine/src              # Engine headers
    ${CMAKE_SOURCE_DIR}/PAINEngine/src/PAINEngine
    ${CMAKE_SOURCE_DIR}/PAINEngine/vendor          # All vendor headers
    ${CMAKE_SOURCE_DIR}/PAINEngine/vendor/spdlog/include
    ${CMAKE_SOURCE_DIR}/PAINEngine/vendor/glm
    ${CMAKE_SOURCE_DIR}/PAINEngine/vendor/GLFW/include
    ${CMAKE_SOURCE_DIR}/PAINEngine/vendor/ImGui
    ${CMAKE_SOURCE_DIR}/PAINEngine/vendor/GL
    ${CMAKE_SOURCE_DIR}/PAINEngine/vendor/Jolt
    ${CMAKE_SOURCE_DIR}/PAINEngine/vendor/Lua
    ${CMAKE_SOURCE_DIR}/PAINEngine/vendor/freetype/include
)

# -------------------------
# Set library output folder
# -------------------------
set_target_properties(PAINEngine PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

# -------------------------
# Link libraries
# -------------------------
# Note: We now link FMOD using the ${FMOD_LIBS} variable from the root CMakeLists.txt
target_link_libraries(PAINEngine PUBLIC
    # FMOD Libraries (handled by root script)
    ${FMOD_LIBS}

    # Other Dependencies
    "${CMAKE_SOURCE_DIR}/PAINEngine/dependencies/lib/glfw3.lib"
    "${CMAKE_SOURCE_DIR}/PAINEngine/dependencies/lib/glew32.lib"
    "${CMAKE_SOURCE_DIR}/PAINEngine/dependencies/lib/freetype.lib"
    "${CMAKE_SOURCE_DIR}/PAINEngine/dependencies/lib/Lua.lib"
    "${CMAKE_SOURCE_DIR}/PAINEngine/dependencies/lib/Jolt_Release.lib"
    "${CMAKE_SOURCE_DIR}/PAINEngine/dependencies/lib/Jolt_Debug.lib"
    opengl32
)

# -------------------------
# Compiler options
# -------------------------
if(MSVC)
    target_compile_options(PAINEngine PUBLIC /utf-8)
endif()

# -------------------------
# Platform macros
# -------------------------
if(WIN32)
    target_compile_definitions(PAINEngine PUBLIC PN_PLATFORM_WINDOWS)
endif()