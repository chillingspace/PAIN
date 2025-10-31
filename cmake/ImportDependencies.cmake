include(FetchContent)

macro(importDependencies)
    message(STATUS "Importing project dependencies with FetchContent...")

    # ImGui
    FetchContent_Declare(
      imgui
      GIT_REPOSITORY https://github.com/ocornut/imgui.git
      GIT_TAG v1.91.9-docking
    )
    FetchContent_MakeAvailable(imgui)
    
    # ImGuizmo
    FetchContent_Declare(
      imguizmo
      GIT_REPOSITORY https://github.com/CedricGuillemet/ImGuizmo.git
      GIT_TAG master
    )
    FetchContent_MakeAvailable(imguizmo)


    # GLM
    FetchContent_Declare(
      glm
      GIT_REPOSITORY https://github.com/g-truc/glm.git
      GIT_TAG 1.0.2
    )
    FetchContent_MakeAvailable(glm)

    # spdlog
    FetchContent_Declare(
      spdlog
      GIT_REPOSITORY https://github.com/gabime/spdlog.git
      GIT_TAG v1.16.0
    )
    FetchContent_MakeAvailable(spdlog)

    # entt
    FetchContent_Declare(
      entt
      GIT_REPOSITORY https://github.com/skypjack/entt.git
      GIT_TAG v3.15.0
    )
    FetchContent_MakeAvailable(entt)

    # Jolt Physics
    FetchContent_Declare(
      Jolt
      GIT_REPOSITORY https://github.com/jrouwe/JoltPhysics.git
      GIT_TAG v5.4.0
    )
    FetchContent_MakeAvailable(Jolt)

    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)               # build static lib
    set(CPP_RTTI_ENABLED OFF CACHE BOOL "" FORCE)                # Jolt default: no RTTI
    # set(JPH_USE_STD_VECTOR OFF CACHE BOOL "" FORCE)            # keep Jolt's Array by default
    # set(DEBUG_RENDERER_IN_DEBUG_AND_RELEASE ON CACHE BOOL "" FORCE) # if you want debug draw in Debug/Release
    # set(DEBUG_RENDERER_IN_DISTRIBUTION OFF CACHE BOOL "" FORCE)
    add_subdirectory("${jolt_SOURCE_DIR}/Build" EXCLUDE_FROM_ALL)
    set_property(TARGET Jolt PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

    # sol2
    FetchContent_Declare(
      sol2
      GIT_REPOSITORY https://github.com/ThePhD/sol2.git
      GIT_TAG v3.3.1
    )
    FetchContent_MakeAvailable(sol2)

    # lua
    FetchContent_Declare(
      lua
      GIT_REPOSITORY https://github.com/lua/lua.git
      GIT_TAG v5.4.8
    )
    FetchContent_Populate(lua)

    # Ensure wrapper is present in fetched dir
    file(COPY "${CMAKE_SOURCE_DIR}/vendor/lua/CMakeLists.txt"
         DESTINATION "${lua_SOURCE_DIR}")

    add_subdirectory(${lua_SOURCE_DIR})

    # nlohmann_json
    FetchContent_Declare(
      nlohmann_json
      GIT_REPOSITORY https://github.com/nlohmann/json.git
      GIT_TAG v3.12.0
    )
    FetchContent_MakeAvailable(nlohmann_json)

    # reflcpp
    FetchContent_Declare(
      reflcpp
      GIT_REPOSITORY https://github.com/veselink1/refl-cpp.git
      GIT_TAG v0.12.4
    )
    FetchContent_MakeAvailable(reflcpp)

    # assimp
    FetchContent_Declare(
      assimp
      GIT_REPOSITORY https://github.com/assimp/assimp.git
      GIT_TAG v6.0.2
    )
    FetchContent_MakeAvailable(assimp)

    # FreeType
    FetchContent_Declare(
      freetype
      GIT_REPOSITORY https://github.com/freetype/freetype.git
      GIT_TAG VER-2-13-3  # Latest stable release
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

        # ASTC Encoder ( Exe )
        set(ASTCENC_CLI ON CACHE BOOL "" FORCE)
        set(ASTCENC_ISA_AVX2 ON CACHE BOOL "" FORCE)
        set(ASTCENC_ISA_SSE2 OFF CACHE BOOL "" FORCE)
        set(ASTCENC_ISA_NEON OFF CACHE BOOL "" FORCE)
        set(ASTCENC_ISA_NATIVE OFF CACHE BOOL "" FORCE)
        FetchContent_Declare(
          astc_encoder
          GIT_REPOSITORY https://github.com/ARM-software/astc-encoder.git
          GIT_TAG 5.3.0
        )
        FetchContent_MakeAvailable(astc_encoder)

        if(TARGET astcenc-avx2)
            message(STATUS "astcenc post build command added")
            add_custom_target(copy_astcenc_bin ALL
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                $<TARGET_FILE_DIR:astcenc-avx2>
                ${ASSETS_TOOLS_OUTPUT_DIR}/astc
                DEPENDS astcenc-avx2
                COMMENT "Copying astcenc-avx.exe, .pdb, .dll and all other files to asset tools output directory")
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

        # ASTC Encoder ( Static Lib )

        # Set ASTCENC build flags for Android ARM
        if(ANDROID_ABI STREQUAL "armeabi-v7a" OR ANDROID_ABI STREQUAL "arm64-v8a")
            set(ASTCENC_ISA_NEON ON CACHE BOOL "" FORCE)
            set(ASTCENC_ISA_AVX2 OFF CACHE BOOL "" FORCE)
            set(ASTCENC_ISA_NATIVE OFF CACHE BOOL "" FORCE)
            set(ASTCENC_ISA_SSE2 OFF CACHE BOOL "" FORCE)
        elseif(ANDROID_ABI STREQUAL "x86_64" OR ANDROID_ABI STREQUAL "x86")
            set(ASTCENC_ISA_NEON OFF CACHE BOOL "" FORCE)
            set(ASTCENC_ISA_AVX2 ON CACHE BOOL "" FORCE)
            set(ASTCENC_ISA_NATIVE OFF CACHE BOOL "" FORCE)
            set(ASTCENC_ISA_SSE2 OFF CACHE BOOL "" FORCE)
        endif()

        set(ASTCENC_DECOMPRESSOR ON CACHE BOOL "" FORCE)
        set(ASTCENC_CLI OFF CACHE BOOL "" FORCE)

        FetchContent_Declare(
          astc_encoder
          GIT_REPOSITORY https://github.com/ARM-software/astc-encoder.git
          GIT_TAG 5.3.0
        )
        FetchContent_MakeAvailable(astc_encoder)

        if(TARGET astcdec-neon-static)
            add_library(astc_encoder::lib ALIAS astcdec-neon-static)
            message(STATUS "libastcenc-neon-static added as astc_encoder::lib")
        elseif(TARGET astcdec-avx2-static)
            add_library(astc_encoder::lib ALIAS astcdec-avx2-static)
            message(STATUS "libastcenc-avx2-static added as astc_encoder::lib")
        else()
            message(FATAL_ERROR "No astcenc static library target found! Check ISA settings and that FetchContent_MakeAvailable completed successfully.")
        endif()
    endif()

endmacro()
