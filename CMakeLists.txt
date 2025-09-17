# Root CMakeLists.txt

cmake_minimum_required(VERSION 3.16)
project(PAIN)

# Set the CMake policy version for the entire project.
cmake_policy(VERSION 3.16)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Set Output Directories ---
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/${CMAKE_BUILD_TYPE})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/${CMAKE_BUILD_TYPE})

# --- Dependency Management ---
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(ImportDependencies)
importDependencies()

# --- Define PAINEngine Library Target ---
add_library(PAINEngine STATIC)

# Gather all engine source files recursively.
file(GLOB_RECURSE ENGINE_SOURCES "source/PAINEngine/*.cpp")
target_sources(PAINEngine PRIVATE ${ENGINE_SOURCES})

# PUBLIC: This target and anything linking to it needs these include paths.
# The 'source' directory is the root for all #includes. This is the main fix.
target_include_directories(PAINEngine PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/source"
    "${spdlog_SOURCE_DIR}/include"
    "${glm_SOURCE_DIR}"
)

# Specify the precompiled header relative to the include path.
target_precompile_headers(PAINEngine PRIVATE "PAINEngine/PAINEngine/pch.h")

# --- Define Game Executable Target ---
add_executable(Game "source/Game/Game.cpp")

# --- Link Game to Engine ---
# This inherits all of PAINEngine's PUBLIC properties.
target_link_libraries(Game PRIVATE PAINEngine)

# --- Windows-Specific Configuration ---
if(NOT ANDROID)
    message(STATUS "Configuring for Windows...")

    # Set compile definitions for the engine. PUBLIC propagates them.
    target_compile_definitions(PAINEngine PUBLIC GLEW_STATIC PN_PLATFORM_WINDOWS)

    # --- FMOD Configuration using IMPORTED targets ---
    add_library(fmod_core SHARED IMPORTED GLOBAL)
    set_target_properties(fmod_core PROPERTIES
        IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/vendor/FMOD/windows/api/core/lib/x64/fmod.dll"
        IMPORTED_IMPLIB "${CMAKE_SOURCE_DIR}/vendor/FMOD/windows/api/core/lib/x64/fmod_vc.lib"
    )
    add_library(fmod_core_log SHARED IMPORTED GLOBAL)
    set_target_properties(fmod_core_log PROPERTIES
        IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/vendor/FMOD/windows/api/core/lib/x64/fmodL.dll"
        IMPORTED_IMPLIB "${CMAKE_SOURCE_DIR}/vendor/FMOD/windows/api/core/lib/x64/fmodL_vc.lib"
    )
    target_include_directories(PAINEngine PUBLIC "${CMAKE_SOURCE_DIR}/vendor/FMOD/windows/api/core/inc")

    # PUBLIC dependencies are needed by PAINEngine and propagated to Game.
    target_link_libraries(PAINEngine PUBLIC
        spdlog::spdlog
        imgui
        glfw
        glew_s
        opengl32
        fmod_core
        fmod_core_log
    )

    # --- Post-Build Commands ---
    add_custom_command(TARGET Game POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:fmod_core>" "$<TARGET_FILE_DIR:Game>"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:fmod_core_log>" "$<TARGET_FILE_DIR:Game>"
        COMMENT "Copying FMOD runtime DLLs..."
    )
    add_custom_command(TARGET Game POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different "${CMAKE_SOURCE_DIR}/assets" "$<TARGET_FILE_DIR:Game>/assets"
        COMMENT "Copying game assets..."
    )
    set_target_properties(Game PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:Game>")
endif()

set_property(DIRECTORY . PROPERTY VS_STARTUP_PROJECT Game)