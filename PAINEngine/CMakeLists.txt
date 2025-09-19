cmake_minimum_required(VERSION 3.25)
project(PAINEngine)

# -------------------------
# Collect Engine Sources
# -------------------------
file(GLOB_RECURSE ENGINE_SRC
    "${CMAKE_CURRENT_SOURCE_DIR}/src/PAINEngine/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/PAINEngine/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/PAINEngine/*.h"
)

# -------------------------
# Build Engine as a Static Library
# -------------------------
add_library(PAINEngine STATIC ${ENGINE_SRC})

# -------------------------
# Compile ImGui Backends Into Engine Target
# -------------------------

# 1) Link vendor targets
target_link_libraries(PAINEngine
  PUBLIC  glm
  PUBLIC  nlohmann_json
  PUBLIC  spdlog_header_only
  PUBLIC  imgui::imgui
  PUBLIC  Jolt::Jolt
)

# 2) ImGui backends compiled into the engine
target_sources(PAINEngine PRIVATE
  ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)
if (ANDROID)
  target_sources(PAINEngine PRIVATE ${IMGUI_DIR}/backends/imgui_impl_android.cpp)
  target_compile_definitions(PAINEngine PRIVATE IMGUI_IMPL_OPENGL_ES3=1)
else()
  target_sources(PAINEngine PRIVATE ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp)
endif()
target_include_directories(PAINEngine PRIVATE ${IMGUI_DIR} ${IMGUI_DIR}/backends)

# 3) Platform GL
if (WIN32 AND NOT ANDROID)
  target_link_libraries(PAINEngine PUBLIC OpenGL::GL glfw)
else() # ANDROID
  # NDK system libs (resolve by name)
  target_link_libraries(PAINEngine PUBLIC EGL GLESv3 android log)
  target_compile_definitions(PAINEngine PRIVATE IMGUI_IMPL_OPENGL_ES3=1)
endif()

# (Optional) any extra includes you keep under vendor/GL for <GL/*.h>
target_link_libraries(PAINEngine PUBLIC gl_headers)

# Link Jolt
target_link_libraries(PAINEngine PUBLIC Jolt)

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
    ${CMAKE_SOURCE_DIR}/PAINEngine/vendor
    
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

# -------------------------
# Organize files in VS Solution Explorer
# -------------------------

# Engine sources
foreach(file IN LISTS ENGINE_SRC)
    file(RELATIVE_PATH relPath "${CMAKE_CURRENT_SOURCE_DIR}/src" "${file}")
    get_filename_component(groupPath "${relPath}" PATH)
    string(REPLACE "/" "\\" groupPath "${groupPath}")
    source_group("${groupPath}" FILES "${file}")
endforeach()
