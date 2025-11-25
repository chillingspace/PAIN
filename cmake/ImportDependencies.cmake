include(FetchContent)

macro(importDependencies)
    message(STATUS "Importing project dependencies with FetchContent...")

    # ImGui
    FetchContent_Declare(
      imgui
      GIT_REPOSITORY https://github.com/ocornut/imgui.git
      GIT_TAG v1.91.9-docking
      GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(imgui)
    
    # ImGuizmo
    FetchContent_Declare(
      imguizmo
      GIT_REPOSITORY https://github.com/CedricGuillemet/ImGuizmo.git
      GIT_TAG master
      GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(imguizmo)


    # GLM
    FetchContent_Declare(
      glm
      GIT_REPOSITORY https://github.com/g-truc/glm.git
      GIT_TAG 1.0.2
      GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(glm)

    # spdlog
    FetchContent_Declare(
      spdlog
      GIT_REPOSITORY https://github.com/gabime/spdlog.git
      GIT_TAG v1.16.0
      GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(spdlog)

    # entt
    FetchContent_Declare(
      entt
      GIT_REPOSITORY https://github.com/skypjack/entt.git
      GIT_TAG v3.15.0
      GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(entt)

    # Jolt Physics
if(ANDROID)
    # CRITICAL: Set these BEFORE fetching/configuring Jolt
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(CPP_RTTI_ENABLED OFF CACHE BOOL "" FORCE)
    set(ENABLE_ASSERTS OFF CACHE BOOL "" FORCE)
    set(ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
    
    # Prevent Jolt from enabling debug features
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DJPH_DISABLE_ASSERTS -DNDEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DJPH_DISABLE_ASSERTS -DNDEBUG")
    
    message(STATUS "Android: Configuring Jolt with asserts disabled")
else()
    # Windows/Desktop: Normal configuration
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(CPP_RTTI_ENABLED OFF CACHE BOOL "" FORCE)
endif()

FetchContent_Declare(
  Jolt
  GIT_REPOSITORY https://github.com/jrouwe/JoltPhysics.git
  GIT_TAG v5.4.0
  GIT_SHALLOW TRUE
)

# Use FetchContent to populate (download) but don't call MakeAvailable yet
FetchContent_GetProperties(Jolt)
if(NOT jolt_POPULATED)
    FetchContent_MakeAvailable(Jolt)
    
    # Now add the subdirectory with our configured options
    add_subdirectory("${jolt_SOURCE_DIR}/Build" ${jolt_BINARY_DIR} EXCLUDE_FROM_ALL)
    
    # Set MSVC runtime library
    if(MSVC AND TARGET Jolt)
        set_property(TARGET Jolt PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
    endif()
    
    # Extra safety: add compile definitions directly to Jolt target
    if(ANDROID AND TARGET Jolt)
        target_compile_definitions(Jolt PUBLIC
            JPH_DISABLE_ASSERTS
            JPH_PROFILE_ENABLED=1
            NDEBUG
        )
        message(STATUS "Jolt configured for Android (assertions disabled)")
    endif()
endif()

    # sol2
    FetchContent_Declare(
      sol2
      GIT_REPOSITORY https://github.com/ThePhD/sol2.git
      GIT_TAG v3.3.1
      GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(sol2)

    #add_library(sol2::sol2 INTERFACE IMPORTED)
    #set_target_properties(sol2::sol2 PROPERTIES
    #  INTERFACE_INCLUDE_DIRECTORIES "${sol2_SOURCE_DIR}/include"
    #)

    # lua
    FetchContent_Declare(
      lua
      GIT_REPOSITORY https://github.com/lua/lua.git
      GIT_TAG v5.4.8
      GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(lua)

    # Ensure wrapper is present in fetched dir
    file(COPY "${CMAKE_SOURCE_DIR}/vendor/lua/CMakeLists.txt"
         DESTINATION "${lua_SOURCE_DIR}")

    add_subdirectory(${lua_SOURCE_DIR})

    # nlohmann_json
    FetchContent_Declare(
      nlohmann_json
      GIT_REPOSITORY https://github.com/nlohmann/json.git
      GIT_TAG v3.12.0
      GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(nlohmann_json)

    # reflcpp
    FetchContent_Declare(
      reflcpp
      GIT_REPOSITORY https://github.com/veselink1/refl-cpp.git
      GIT_TAG v0.12.4
      GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(reflcpp)

    # assimp
    FetchContent_Declare(
      assimp
      GIT_REPOSITORY https://github.com/assimp/assimp.git
      GIT_TAG v6.0.2
      GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(assimp)

    # FreeType
    FetchContent_Declare(
      freetype
      GIT_REPOSITORY https://github.com/freetype/freetype.git
      GIT_TAG VER-2-13-3  # Latest stable release
      GIT_SHALLOW TRUE
    )
    # Disable FreeType's unnecessary features to speed up build
    set(FT_DISABLE_ZLIB ON CACHE BOOL "" FORCE)
    set(FT_DISABLE_BZIP2 ON CACHE BOOL "" FORCE)
    set(FT_DISABLE_PNG ON CACHE BOOL "" FORCE)
    set(FT_DISABLE_HARFBUZZ ON CACHE BOOL "" FORCE)
    set(FT_DISABLE_BROTLI ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(freetype)


    set(ASSETS_TOOLS_OUTPUT_DIR ${CMAKE_BINARY_DIR}/Tools CACHE PATH "Folder for placing asset tools CLI")

    # Exclude From Android
    if(NOT ANDROID)

        # GLFW
        FetchContent_Declare(
            glfw
            GIT_REPOSITORY https://github.com/glfw/glfw.git
            GIT_TAG 3.4
            GIT_SHALLOW TRUE
        )
        set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
        set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
        set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(glfw)

        # File Watcher
        FetchContent_Declare(
          filewatch
          GIT_REPOSITORY https://github.com/ThomasMonkman/FileWatch.git
          GIT_TAG a59891baf375b73ff28144973a6fafd3fe40aa21 # Tagged to a stable commit
        )

        # Populate the dependency
        FetchContent_Populate(filewatch)

        # Patch the CMakeLists.txt immediately after popuplate
        if(EXISTS "${filewatch_SOURCE_DIR}/CMakeLists.txt")
            file(READ "${filewatch_SOURCE_DIR}/CMakeLists.txt" FILEWATCH_CMAKE)
            string(REGEX REPLACE "cmake_minimum_required\\([^)]+\\)" "cmake_minimum_required(VERSION 3.22)" FILEWATCH_CMAKE "${FILEWATCH_CMAKE}")
            file(WRITE "${filewatch_SOURCE_DIR}/CMakeLists.txt" "${FILEWATCH_CMAKE}")
        endif()

        # Manually add the subdirectory
        add_subdirectory(${filewatch_SOURCE_DIR} ${filewatch_BINARY_DIR})

        # Glew
        FetchContent_Declare(
            glew
            GIT_REPOSITORY https://github.com/omniavinco/glew-cmake.git
            GIT_TAG d06782b910213d675925e6e51a69ad0fd1fe1f23  # Tagged to a stable commit
        )

        # Disable shared builds if needed
        set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libs" FORCE)
        FetchContent_MakeAvailable(glew)

        # include directories
        include_directories(${glew_SOURCE_DIR}/include)

        # stb (header-only)
        FetchContent_Declare(
          stb
          GIT_REPOSITORY https://github.com/nothings/stb.git
          GIT_TAG 4c5645949723fbb9c060c3f94157331fa1d043bf # Tagged to a stable commit
        )
        FetchContent_MakeAvailable(stb)

        # Cuttlefish
        FetchContent_Declare(
            cuttlefish
            GIT_REPOSITORY https://github.com/akb825/Cuttlefish.git
            GIT_TAG v2.9.0
            GIT_SUBMODULES "."
            GIT_SHALLOW TRUE
        )
        FetchContent_MakeAvailable(cuttlefish)

        if(TARGET cuttlefish)
            message(STATUS "Cuttlefish post build command added")
            add_custom_target(copy_cuttlefish_bin ALL
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                $<TARGET_FILE_DIR:cuttlefish>
                ${ASSETS_TOOLS_OUTPUT_DIR}/cuttlefish
                DEPENDS cuttlefish
                COMMENT "Copying cuttlefish.exe, .pdb, .dll and all other files to asset tools output directory")
        endif()

        # Copy FFMPEG
        add_custom_target(copy_ffmpeg_bin ALL
            COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_SOURCE_DIR}/vendor/ffmpeg/ffmpeg.exe
                ${ASSETS_TOOLS_OUTPUT_DIR}/ffmpeg/ffmpeg.exe
            COMMENT "Copying ffmpeg.exe to asset tools output directory")
    endif()

    # Android only
    if(ANDROID)


        FetchContent_Declare(
            ktx
            GIT_REPOSITORY https://github.com/KhronosGroup/KTX-Software.git
            GIT_TAG        v4.4.2
        )
        FetchContent_MakeAvailable(ktx)
    endif()

endmacro()
