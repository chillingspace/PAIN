cmake_minimum_required(VERSION 3.25)
project(PAINEngine)

# -------------------------
# Collect Engine Sources
# -------------------------
file(GLOB_RECURSE ENGINE_SRC
    "${CMAKE_CURRENT_SOURCE_DIR}/src/PAINEngine/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/PAINEngine/*.c"
)

# -------------------------
# Define ImGui Sources
# -------------------------
set(IMGUI_SOURCES
    ${CMAKE_SOURCE_DIR}/PAINEngine/vendor/ImGui/src/imgui.cpp
    ${CMAKE_SOURCE_DIR}/PAINEngine/vendor/ImGui/src/imgui_demo.cpp
    ${CMAKE_SOURCE_DIR}/PAINEngine/vendor/ImGui/src/imgui_draw.cpp
    ${CMAKE_SOURCE_DIR}/PAINEngine/vendor/ImGui/src/imgui_tables.cpp
    ${CMAKE_SOURCE_DIR}/PAINEngine/vendor/ImGui/src/imgui_widgets.cpp
    ${CMAKE_SOURCE_DIR}/PAINEngine/vendor/ImGui/src/imgui_impl_glfw.cpp
    ${CMAKE_SOURCE_DIR}/PAINEngine/vendor/ImGui/src/imgui_impl_opengl3.cpp
)

# -------------------------
# Build ImGui as a Static Library
# -------------------------
add_library(imgui STATIC ${IMGUI_SOURCES})

# Add the vendor directory to the include path for the imgui target
target_include_directories(imgui PUBLIC
    "${CMAKE_SOURCE_DIR}/PAINEngine/vendor"
)

# Add required compile definitions directly to the imgui target
target_compile_definitions(imgui PUBLIC
    IMGUI_IMPL_OPENGL_LOADER_GLEW
    IMGUI_DEFINE_MATH_OPERATORS
)

# -------------------------
# Build Engine as a Static Library
# -------------------------
add_library(PAINEngine STATIC ${ENGINE_SRC})

# -------------------------
# Precompiled Header
# -------------------------
target_precompile_headers(PAINEngine
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/PAINEngine/pch.h"
)

# -------------------------
# Include Directories
# -------------------------
target_include_directories(PAINEngine PUBLIC
    ${CMAKE_SOURCE_DIR}/PAINEngine/src
    ${CMAKE_SOURCE_DIR}/PAINEngine/src/PAINEngine
    
    # Treat vendor folders as SYSTEM to suppress their warnings
    SYSTEM ${CMAKE_SOURCE_DIR}/PAINEngine/vendor/spdlog/include
    SYSTEM ${CMAKE_SOURCE_DIR}/PAINEngine/vendor/glm
    SYSTEM ${CMAKE_SOURCE_DIR}/PAINEngine/vendor/GLFW/include
    SYSTEM ${CMAKE_SOURCE_DIR}/PAINEngine/vendor/GL
    SYSTEM ${CMAKE_SOURCE_DIR}/PAINEngine/vendor/Jolt
    SYSTEM ${CMAKE_SOURCE_DIR}/PAINEngine/vendor/Lua
    SYSTEM ${CMAKE_SOURCE_DIR}/PAINEngine/vendor/freetype/include
)

# -------------------------
# Set Library Output Folder
# -------------------------
set_target_properties(PAINEngine PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

# -------------------------
# Link Libraries
# -------------------------
target_link_libraries(PAINEngine PUBLIC
    imgui
    ${FMOD_LIBS}
    "${CMAKE_SOURCE_DIR}/PAINEngine/dependencies/lib/glfw3.lib"
    "${CMAKE_SOURCE_DIR}/PAINEngine/dependencies/lib/glew32s.lib"
    "${CMAKE_SOURCE_DIR}/PAINEngine/dependencies/lib/freetype.lib"
    "${CMAKE_SOURCE_DIR}/PAINEngine/dependencies/lib/Lua.lib"
    "${CMAKE_SOURCE_DIR}/PAINEngine/dependencies/lib/Jolt_Release.lib"
    "${CMAKE_SOURCE_DIR}/PAINEngine/dependencies/lib/Jolt_Debug.lib"
    opengl32
)

# -------------------------
# Compiler Options
# -------------------------
if(MSVC)
    target_compile_options(PAINEngine PUBLIC /utf-8)
endif()

# -------------------------
# Platform and Dependency Macros
# -------------------------
target_compile_definitions(PAINEngine PUBLIC
    PN_PLATFORM_WINDOWS
    GLEW_STATIC
)