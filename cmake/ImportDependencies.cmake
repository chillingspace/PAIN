# cmake/ImportDependencies.cmake

# This script finds local libraries in the /libs folder and defines them as CMake targets.

# --- Platform Detection ---
if(CMAKE_SYSTEM_NAME STREQUAL "Android")
    set(ANDROID TRUE)
endif()

# ===================================================================
#  Header-Only Libraries
# ===================================================================

add_library(glm INTERFACE)
target_include_directories(glm SYSTEM INTERFACE ${CMAKE_SOURCE_DIR}/libs/vendor/glm)

add_library(spdlog INTERFACE)
target_include_directories(spdlog SYSTEM INTERFACE ${CMAKE_SOURCE_DIR}/libs/vendor/spdlog/include)

# ===================================================================
#  Compiled Libraries (Platform-Specific)
# ===================================================================

if(NOT ANDROID)
    # --- Windows-Specific Libraries ---

    add_library(glfw STATIC IMPORTED)
    set_target_properties(glfw PROPERTIES
        IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/libs/dependencies/lib/glfw3.lib"
        INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/libs/vendor/GLFW"
    )

    # --- GLEW (OpenGL Extensions) ---
    # CORRECTED: This now points to glew32s.lib for all configurations
    # as the debug version (glew32sd.lib) does not exist in the project.
    add_library(glew_static STATIC IMPORTED)
    set_target_properties(glew_static PROPERTIES
        IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/libs/dependencies/lib/glew32s.lib"
        INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/libs/vendor/GL"
    )
    target_compile_definitions(glew_static INTERFACE GLEW_STATIC)

    # --- FMOD (Audio Engine) ---
    add_library(fmod SHARED IMPORTED)
    set_target_properties(fmod PROPERTIES
        IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/libs/dependencies/shared/fmod.dll"
        IMPORTED_IMPLIB "${CMAKE_SOURCE_DIR}/libs/dependencies/lib/fmod_vc.lib"
    )
    add_library(fmodL SHARED IMPORTED)
    set_target_properties(fmodL PROPERTIES
        IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/libs/dependencies/shared/fmodL.dll"
        IMPORTED_IMPLIB "${CMAKE_SOURCE_DIR}/libs/dependencies/lib/fmodL_vc.lib"
    )
    add_library(fmod_core INTERFACE)
    target_link_libraries(fmod_core INTERFACE
        $<$<CONFIG:Debug>:fmodL>
        $<$<NOT:$<CONFIG:Debug>>:fmod>
    )
    target_include_directories(fmod_core SYSTEM INTERFACE ${CMAKE_SOURCE_DIR}/libs/vendor/FMOD/windows/api/core/inc)

    # --- Jolt (Physics Engine) ---
    add_library(jolt INTERFACE)
    target_include_directories(jolt SYSTEM INTERFACE ${CMAKE_SOURCE_DIR}/libs/vendor/Jolt)
    target_link_libraries(jolt INTERFACE
        $<$<CONFIG:Debug>:${CMAKE_SOURCE_DIR}/libs/dependencies/lib/Jolt_Debug.lib>
        $<$<NOT:$<CONFIG:Debug>>:${CMAKE_SOURCE_DIR}/libs/dependencies/lib/Jolt_Release.lib>
    )
endif()

# --- ImGui (Built from source) ---
add_library(imgui STATIC
    "${CMAKE_SOURCE_DIR}/libs/vendor/ImGui/src/imgui.cpp"
    "${CMAKE_SOURCE_DIR}/libs/vendor/ImGui/src/imgui_demo.cpp"
    "${CMAKE_SOURCE_DIR}/libs/vendor/ImGui/src/imgui_draw.cpp"
    "${CMAKE_SOURCE_DIR}/libs/vendor/ImGui/src/imgui_tables.cpp"
    "${CMAKE_SOURCE_DIR}/libs/vendor/ImGui/src/imgui_widgets.cpp"
)
target_include_directories(imgui PUBLIC
    "${CMAKE_SOURCE_DIR}/libs/vendor"
)

# Add platform-specific backend source files
if(ANDROID)
    target_sources(imgui PRIVATE
        "${CMAKE_SOURCE_DIR}/libs/vendor/ImGui/src/imgui_impl_opengl3.cpp"
    )
else() # Windows
    target_sources(imgui PRIVATE
        "${CMAKE_SOURCE_DIR}/libs/vendor/ImGui/src/imgui_impl_glfw.cpp"
        "${CMAKE_SOURCE_DIR}/libs/vendor/ImGui/src/imgui_impl_opengl3.cpp"
    )
    target_compile_definitions(imgui PRIVATE IMGUI_IMPL_OPENGL_LOADER_GLEW)
endif()