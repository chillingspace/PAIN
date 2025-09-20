cmake_minimum_required(VERSION 3.25)
project(PAINEngine)

# ======================= Engine Sources  =========================

file(GLOB_RECURSE ENGINE_SRC
    "${CMAKE_CURRENT_SOURCE_DIR}/src/PAINEngine/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/PAINEngine/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/PAINEngine/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/PAINEngine/*.hpp"
)

add_library(PAINEngine STATIC ${ENGINE_SRC})

# Public headers
target_include_directories(PAINEngine
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/PAINEngine
)

# ======================= Precompiled Headers  =========================

target_precompile_headers(PAINEngine
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/PAINEngine/pch.h"
)

# ======================= Link Vendors  =========================

# target link vendors
target_link_libraries(PAINEngine
    PRIVATE  
        glm
        nlohmann_json
        imgui::imgui
        Jolt
        gl_headers
        fmod
        fmodL
)

target_link_libraries(PAINEngine
    PUBLIC  
        spdlog_header_only
)

# ImGui backends compiled into the engine
target_sources(PAINEngine PRIVATE
  ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

# Android ImGui VS Window ImGui
if (ANDROID)
    target_sources(PAINEngine PRIVATE ${IMGUI_DIR}/backends/imgui_impl_android.cpp)
    target_compile_definitions(PAINEngine PRIVATE IMGUI_IMPL_OPENGL_ES3=1)
else()
set_source_files_properties(${IMGUI_GL3}
  PROPERTIES COMPILE_DEFINITIONS "IMGUI_IMPL_OPENGL_LOADER_CUSTOM=1")
    target_sources(PAINEngine PRIVATE ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp)
endif()
    target_include_directories(PAINEngine PRIVATE ${IMGUI_DIR} ${IMGUI_DIR}/backends)

# Platform GL
if (WIN32 AND NOT ANDROID)
    target_link_libraries(PAINEngine PUBLIC OpenGL::GL glfw)
    target_compile_definitions(PAINEngine PUBLIC PN_PLATFORM_WINDOWS GLEW_STATIC)
else()
    target_link_libraries(PAINEngine PUBLIC EGL GLESv3 android log)
    target_compile_definitions(PAINEngine PRIVATE IMGUI_IMPL_OPENGL_ES3=1)
endif()

# ======================= Engine Output  =========================

set_target_properties(PAINEngine PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

# ======================= Compiler Options  =========================

if(MSVC)
    target_compile_options(PAINEngine PUBLIC /utf-8)
endif()

# ======================= VS FIlters  =========================

# Engine sources
foreach(file IN LISTS ENGINE_SRC)
    file(RELATIVE_PATH relPath "${CMAKE_CURRENT_SOURCE_DIR}/src" "${file}")
    get_filename_component(groupPath "${relPath}" PATH)
    string(REPLACE "/" "\\" groupPath "${groupPath}")
    source_group("${groupPath}" FILES "${file}")
endforeach()
