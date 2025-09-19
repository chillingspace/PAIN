include(FetchContent)

# Macro to import GLFW
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

# Macro to import glm
macro(import_glm)
    if(NOT TARGET glm)
        FetchContent_Declare(
            glm
            GIT_REPOSITORY https://github.com/g-truc/glm.git
            GIT_TAG 0.9.9.8
        )
        FetchContent_MakeAvailable(glm)
    endif()
endmacro()

# Macro to import glew
macro(import_glew)
    if(NOT ANDROID AND NOT TARGET glew_s)
        FetchContent_Declare(
            glew
            GIT_REPOSITORY https://github.com/nigels-com/glew.git
            GIT_TAG glew-2.2.0
        )
        set(GLEW_BUILD_UTILS OFF CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(glew)
    endif()
endmacro()

# NEW: Macro to import spdlog
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

# NEW: Macro to import Jolt Physics
macro(import_jolt)
    if(NOT TARGET Jolt)
        FetchContent_Declare(
            jolt
            GIT_REPOSITORY https://github.com/jrouwe/JoltPhysics.git
            GIT_TAG v4.0.3
        )
        # Disable parts of Jolt we don't need to speed up the build
        set(JPH_PROFILE_ENABLED OFF CACHE BOOL "" FORCE)
        set(JPH_DEBUG_RENDERER OFF CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(jolt)
    endif()
endmacro()


# Macro to import all dependencies
macro(importDependencies)
    message(STATUS "Importing all dependencies...")
    
    import_glfw()
    import_glm()
    import_glew()
    import_spdlog()
    import_jolt()

    message(STATUS "All dependencies have been imported successfully.")
endmacro()