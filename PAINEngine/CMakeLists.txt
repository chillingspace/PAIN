# Define the library target for the engine
add_library(PAINEngine STATIC)

# Gather all source files from the src directory
file(GLOB_RECURSE PAINEngine_SOURCES "src/*.cpp")
target_sources(PAINEngine PRIVATE ${PAINEngine_SOURCES})

# --- Find Dependencies using vcpkg ---
# vcpkg makes these libraries available to be found by find_package
find_package(spdlog CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(GLEW CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(FMOD CONFIG REQUIRED) # FMOD is now handled by vcpkg in the sample

# --- Include Directories ---
# PUBLIC: Any target that links to PAINEngine will also get these include paths.
target_include_directories(PAINEngine PUBLIC
    "src"
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

# --- Link Libraries ---
# PUBLIC: Any target that links to PAINEngine will also link to these libraries.
target_link_libraries(PAINEngine PUBLIC
    spdlog::spdlog
    glm::glm
    glfw
    GLEW::glew
    imgui::imgui
    FMOD::fmod_static # Use the static library provided by vcpkg
)

# --- Precompiled Header ---
# This significantly speeds up the build.
target_precompile_headers(PAINEngine PRIVATE "PAINEngine/pch.h")

# --- Platform-specific Definitions ---
target_compile_definitions(PAINEngine PUBLIC
    $<$<PLATFORM_ID:Windows>:GLEW_STATIC>
    $<$<PLATFORM_ID:Windows>:PN_PLATFORM_WINDOWS>
    $<$<PLATFORM_ID:Android>:PN_PLATFORM_ANDROID>
)