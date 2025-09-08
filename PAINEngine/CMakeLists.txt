# ========================
# PAINEngine CMake
# ========================

# Collect engine sources
file(GLOB_RECURSE ENGINE_SRC CONFIGURE_DEPENDS src/*.cpp src/*.h)

# Build engine as shared library
add_library(PAINEngine SHARED ${ENGINE_SRC})

# -------------------------
# Include directories
# -------------------------
target_include_directories(PAINEngine PUBLIC
    ${CMAKE_SOURCE_DIR}/PAINEngine/src                  # Engine headers
    ${CMAKE_SOURCE_DIR}/PAINEngine/vendor               # All vendor headers
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
# Library directories
# -------------------------
link_directories(${CMAKE_SOURCE_DIR}/PAINEngine/dependencies/lib)

# -------------------------
# Link libraries
# -------------------------
target_link_libraries(PAINEngine
    "${CMAKE_SOURCE_DIR}/PAINEngine/dependencies/lib/glfw3.lib"
    "${CMAKE_SOURCE_DIR}/PAINEngine/dependencies/lib/glew32.lib"
    "${CMAKE_SOURCE_DIR}/PAINEngine/dependencies/lib/fmod_vc.lib"
    "${CMAKE_SOURCE_DIR}/PAINEngine/dependencies/lib/fmodL_vc.lib"
    "${CMAKE_SOURCE_DIR}/PAINEngine/dependencies/lib/freetype.lib"
    "${CMAKE_SOURCE_DIR}/PAINEngine/dependencies/lib/Lua.lib"
    "${CMAKE_SOURCE_DIR}/PAINEngine/dependencies/lib/Jolt_Release.lib"
    "${CMAKE_SOURCE_DIR}/PAINEngine/dependencies/lib/Jolt_Debug.lib"
)

# -------------------------
# Preprocessor definitions
# -------------------------
# Export macro for DLL
target_compile_definitions(PAINEngine PRIVATE PAINENGINE_EXPORTS)

# Windows platform detection
if(WIN32)
    target_compile_definitions(PAINEngine PUBLIC PN_PLATFORM_WINDOWS PN_BUILD_DLL)
endif()

# Force UTF-8 encoding for spdlog/fmt
if(MSVC)
    target_compile_options(PAINEngine PUBLIC /utf-8)
endif()


# -------------------------
# Copy DLLs after build
# -------------------------
# Copy everything from shared/ into the output bin directory
add_custom_command(TARGET PAINEngine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/PAINEngine/dependencies/shared
        $<TARGET_FILE_DIR:PAINEngine>
)
