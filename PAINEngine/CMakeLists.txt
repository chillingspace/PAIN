project(PAINEngine)

# --- Collect Engine Sources ---
file(GLOB_RECURSE ENGINE_SRC
    "${CMAKE_CURRENT_SOURCE_DIR}/src/PAINEngine/*.cpp"
)

# --- Build Engine Library ---
add_library(PAINEngine STATIC ${ENGINE_SRC})

# --- Set MSVC Compiler Options ---
if(MSVC)
    # /W4 (high warning level), /EHsc (standard exception handling)
    target_compile_options(PAINEngine PRIVATE /W4 /EHsc)
endif()

# --- Precompiled Header ---
target_precompile_headers(PAINEngine
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/PAINEngine/pch.h"
)

# --- Include Directories ---
target_include_directories(PAINEngine PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/PAINEngine"
)

# --- Link Dependencies ---
target_link_libraries(PAINEngine PUBLIC
    # Header-Only
    glm
    spdlog

    # Compiled
    imgui
)

# --- Windows-Specific Dependencies ---
if(NOT ANDROID)
    target_link_libraries(PAINEngine PUBLIC
        glfw
        glew_static
        fmod_core
        jolt
        opengl32
    )
endif()