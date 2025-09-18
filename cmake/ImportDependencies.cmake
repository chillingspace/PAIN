include(FetchContent)

# Macro to import GLFW
macro(import_glfw)
    if(NOT ANDROID AND NOT TARGET glfw)  # Guard to prevent multiple inclusion and skip on Android
        FetchContent_Declare(
            glfw
            GIT_REPOSITORY https://github.com/glfw/glfw.git
            GIT_TAG 3.3.8
        )
        if(NOT glfw_POPULATED)
            set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
            set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
            set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
            FetchContent_Populate(glfw)
        endif()

        add_subdirectory(${glfw_SOURCE_DIR})
        include_directories(${GLFW_SOURCE_DIR}/include)
    endif()
endmacro()

# Macro to import glm
macro(import_glm)
    if(NOT TARGET glm)  # Guard to prevent multiple inclusion
        FetchContent_Declare(
            glm
            GIT_REPOSITORY https://github.com/g-truc/glm.git
            GIT_TAG master
        )
        FetchContent_MakeAvailable(glm)

        include_directories(${glm_SOURCE_DIR})
    endif()
endmacro()

# Macro to import glew
macro(import_glew)
    if(NOT ANDROID AND NOT TARGET glew_s)  # Guard to prevent multiple inclusion and skip on Android
        include(FetchContent)

        FetchContent_Declare(
            glew
            GIT_REPOSITORY https://github.com/omniavinco/glew-cmake.git
            GIT_TAG master  # Or use a specific commit or tag if preferred
        )

        # Disable shared builds if needed
        set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libs" FORCE)
        FetchContent_MakeAvailable(glew)

        # Optional: include directories if needed manually (though target_link_libraries handles it)
        include_directories(${glew_SOURCE_DIR}/include)
    endif()
endmacro()

# Macro to import ImGui
macro(import_imgui)
    if(NOT TARGET imgui)  # Guard to prevent multiple inclusion
        FetchContent_Declare(
            imgui
            GIT_REPOSITORY https://github.com/ocornut/imgui.git
            GIT_TAG v1.90.4
        )
        if(NOT imgui_POPULATED)
            FetchContent_Populate(imgui)
        endif()

        # Create ImGui library manually since it doesn't have CMakeLists.txt
        set(IMGUI_SOURCES
            ${imgui_SOURCE_DIR}/imgui.cpp
            ${imgui_SOURCE_DIR}/imgui_demo.cpp
            ${imgui_SOURCE_DIR}/imgui_draw.cpp
            ${imgui_SOURCE_DIR}/imgui_tables.cpp
            ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        )

        # Add platform-specific backend sources
        if(ANDROID)
            # For Android, we'll use OpenGL ES 3.0 backend
            list(APPEND IMGUI_SOURCES
                ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
            )
        else()
            # For desktop, use GLFW + OpenGL 3 backend
            list(APPEND IMGUI_SOURCES
                ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
                ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
            )
        endif()

        add_library(imgui STATIC ${IMGUI_SOURCES})
        target_include_directories(imgui PUBLIC 
            ${imgui_SOURCE_DIR}
            ${imgui_SOURCE_DIR}/backends
        )
        
        # Link platform-specific libraries
        if(ANDROID)
            target_link_libraries(imgui PUBLIC GLESv3 EGL)
        else()
            target_link_libraries(imgui PUBLIC glfw libglew_static)
        endif()
        
        # Set C++ standard for ImGui (required for constexpr support)
        set_property(TARGET imgui PROPERTY CXX_STANDARD 11)
        set_property(TARGET imgui PROPERTY CXX_STANDARD_REQUIRED ON)
    endif()
endmacro()

# Macro to import reflcpp
macro(import_reflcpp)
    if(NOT TARGET reflcpp)  # Guard to prevent multiple inclusion
        FetchContent_Declare(reflcpp
            GIT_REPOSITORY https://github.com/veselink1/refl-cpp.git
            GIT_TAG v0.12.4
        )
        FetchContent_MakeAvailable(reflcpp)
    endif()
endmacro()

# Macro to import nlohmann-json
macro(import_nlohmann_json)
    if(NOT TARGET nlohmann_json::nlohmann_json)  # Guard to prevent multiple inclusion
        #FetchContent_Declare(nlohmann_json
        #    GIT_REPOSITORY https://github.com/nlohmann/json.git
        #    GIT_TAG v3.11.3
        #)
        FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz)
        FetchContent_MakeAvailable(json)
    endif()
endmacro()

# Macro to import all dependencies
macro(importDependencies)
    message(STATUS "Starting to import dependencies...")

    if(ANDROID)
        message(STATUS "Android platform detected - importing Android-specific dependencies...")
        
        message(STATUS "Importing GLM...")
        import_glm()
        message(STATUS "GLM imported successfully.")

        message(STATUS "Importing ImGui...")
        import_imgui()
        message(STATUS "ImGui imported successfully.")

        message(STATUS "Importing Reflcpp...")
        import_reflcpp()
        message(STATUS "Reflcpp imported successfully.")

        message(STATUS "Importing nlohmann-json...")
        import_nlohmann_json()
        message(STATUS "nlohmann-json imported successfully.")
        
        message(STATUS "Android dependencies imported successfully.")
    else()
        message(STATUS "Desktop platform detected - importing desktop dependencies...")
        
        # message(STATUS "Importing GLFW...")
        # import_glfw()
        # message(STATUS "GLFW imported successfully.")

        message(STATUS "Importing GLM...")
        import_glm()
        message(STATUS "GLM imported successfully.")

        message(STATUS "Importing GLEW...")
        import_glew()
        message(STATUS "GLEW imported successfully.")

        # message(STATUS "Importing ImGui...")
        # import_imgui()
        # message(STATUS "ImGui imported successfully.")

        message(STATUS "Importing Reflcpp...")
        import_reflcpp()
        message(STATUS "Reflcpp imported successfully.")

        message(STATUS "Importing nlohmann-json...")
        import_nlohmann_json()
        message(STATUS "nlohmann-json imported successfully.")
        
        message(STATUS "Desktop dependencies imported successfully.")
    endif()

    message(STATUS "All dependencies have been imported successfully.")
endmacro()