# cmake/ImportDependencies.cmake

include(FetchContent)

# Downloads and configures GLFW (for Windows platform)
macro(import_glfw)
    if(NOT ANDROID AND NOT TARGET glfw)
        FetchContent_Declare(
            glfw
            GIT_REPOSITORY https://github.com/glfw/glfw.git
            GIT_TAG 3.3.8
        )
        set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
        set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
        set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(glfw)
    endif()
endmacro()

# Downloads and configures GLEW (for Windows platform)
macro(import_glew)
    if(NOT ANDROID AND NOT TARGET glew_s)
        FetchContent_Declare(
            glew
            GIT_REPOSITORY https://github.com/nigels-com/glew.git
            GIT_TAG glew-2.2.0
        )
        # Force a static build of GLEW
        set(GLEW_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(glew)
    endif()
endmacro()

# Downloads and configures GLM (header-only, for all platforms)
macro(import_glm)
    if(NOT TARGET glm)
        FetchContent_Declare(
            glm
            GIT_REPOSITORY https://github.com/g-truc/glm.git
            # Updated to a newer tag with a compatible CMake script.
            GIT_TAG 1.0.1
        )
        FetchContent_MakeAvailable(glm)
    endif()
endmacro()

# Downloads and configures ImGui (for all platforms)
macro(import_imgui)
    if(NOT TARGET imgui)
        FetchContent_Declare(
            imgui
            GIT_REPOSITORY https://github.com/ocornut/imgui.git
            GIT_TAG v1.89.9
        )
        set(IMGUI_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(imgui)
    endif()
endmacro()

# Downloads and configures spdlog (header-only, for all platforms)
macro(import_spdlog)
    if(NOT TARGET spdlog)
        FetchContent_Declare(
            spdlog
            GIT_REPOSITORY https://github.com/gabime/spdlog.git
            GIT_TAG v1.12.0
        )
        FetchContent_MakeAvailable(spdlog)
    endif()
endmacro()

# Main macro that executes all dependency imports
macro(importDependencies)
    message(STATUS "Importing project dependencies...")
    import_glfw()
    import_glew()
    import_glm()
    import_imgui()
    import_spdlog()
    message(STATUS "Dependencies imported.")
endmacro()