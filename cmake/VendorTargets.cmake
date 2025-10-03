if (NOT DEFINED VENDOR_DIR)
  set(VENDOR_DIR "${CMAKE_SOURCE_DIR}/vendor")
endif()

# ======================= Header Only Vendors  =========================
add_library(glm INTERFACE)
target_include_directories(glm INTERFACE "${VENDOR_DIR}")

add_library(nlohmann_json INTERFACE)
target_include_directories(nlohmann_json INTERFACE "${VENDOR_DIR}/nlohmann/include")

add_library(spdlog_header_only INTERFACE)
target_include_directories(spdlog_header_only INTERFACE "${VENDOR_DIR}/spdlog/include")

add_library(FileWatch_header_only INTERFACE)
target_include_directories(FileWatch_header_only INTERFACE "${VENDOR_DIR}/FileWatch")

add_library(gl_headers INTERFACE)
target_include_directories(gl_headers INTERFACE "${VENDOR_DIR}/GL")

add_library(gli_headers INTERFACE)
target_include_directories(gli_headers INTERFACE "${VENDOR_DIR}/gli")
target_link_libraries(gli_headers INTERFACE glm) 

# ======================= GLEW Vendor  =========================

if (WIN32 AND NOT ANDROID)
    set(_GLEW_DIR "${CMAKE_SOURCE_DIR}/vendor/glew")

    add_library(_glew STATIC
      "${_GLEW_DIR}/src/glew.c"
    )
    target_include_directories(_glew PUBLIC "${_GLEW_DIR}/include")
    target_compile_definitions(_glew PUBLIC GLEW_STATIC)  # ensure headers use static path

    add_library(GLEW::GLEW ALIAS _glew)
endif()

# ======================= ImGui Vendor  =========================

add_library(imgui STATIC
  "${VENDOR_DIR}/ImGui/imgui.cpp"
  "${VENDOR_DIR}/ImGui/imgui_draw.cpp"
  "${VENDOR_DIR}/ImGui/imgui_tables.cpp"
  "${VENDOR_DIR}/ImGui/imgui_widgets.cpp"
  "${VENDOR_DIR}/ImGui/imgui_demo.cpp"
)
target_include_directories(imgui PUBLIC "${VENDOR_DIR}/ImGui")
add_library(imgui::imgui ALIAS imgui)

# Expose IMGUI dir path to subprojects for backends
set(IMGUI_DIR "${VENDOR_DIR}/ImGui" CACHE PATH "Path to ImGui sources")

# ======================= GLFW Vendor  =========================

if (WIN32 AND NOT ANDROID)
    if (EXISTS "${VENDOR_DIR}/GLFW/CMakeLists.txt")
      set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
      set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
      set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
      add_subdirectory("${VENDOR_DIR}/GLFW" "${CMAKE_BINARY_DIR}/vendor_glfw")
    else()
      add_library(glfw STATIC IMPORTED GLOBAL)
      set_target_properties(glfw PROPERTIES
        IMPORTED_LOCATION             "${VENDOR_DIR}/GLFW/lib/glfw3.lib"
        INTERFACE_INCLUDE_DIRECTORIES "${VENDOR_DIR}/GLFW/include"
      )
    endif()
endif()

# ======================= Jolt Vendor  =========================

# (Optional but recommended) choose Jolt options BEFORE add_subdirectory.
# They�ll become the default values in the Jolt subproject cache.
# See docs for meaning of these flags. :contentReference[oaicite:1]{index=1}
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)               # build static lib
set(CPP_RTTI_ENABLED OFF CACHE BOOL "" FORCE)                # Jolt default: no RTTI
# set(JPH_USE_STD_VECTOR OFF CACHE BOOL "" FORCE)            # keep Jolt's Array by default
# set(DEBUG_RENDERER_IN_DEBUG_AND_RELEASE ON CACHE BOOL "" FORCE) # if you want debug draw in Debug/Release
# set(DEBUG_RENDERER_IN_DISTRIBUTION OFF CACHE BOOL "" FORCE)

# Add the Jolt project (use EXCLUDE_FROM_ALL to avoid building Samples etc. unless asked)
add_subdirectory("${VENDOR_DIR}/Jolt/Build" "${CMAKE_BINARY_DIR}/vendor_jolt" EXCLUDE_FROM_ALL)

set_property(TARGET Jolt PROPERTY
  MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

# ======================= GL Vendor  =========================

if (WIN32 AND NOT ANDROID)
  find_package(OpenGL REQUIRED)
endif()

if (ANDROID)
    # Robustly locate the NDK glue dir (CMake sets one of these)
    set(_NDK "${CMAKE_ANDROID_NDK}")
    if(NOT _NDK AND DEFINED ANDROID_NDK)
        set(_NDK "${ANDROID_NDK}")
    endif()
    if(NOT _NDK)
        message(FATAL_ERROR "Cannot find NDK path (CMAKE_ANDROID_NDK/ANDROID_NDK not set)")
    endif()

    set(NATIVE_APP_GLUE_DIR "${_NDK}/sources/android/native_app_glue")

    add_library(native_app_glue STATIC
            "${NATIVE_APP_GLUE_DIR}/android_native_app_glue.c"
    )
    target_include_directories(native_app_glue PUBLIC
            "${NATIVE_APP_GLUE_DIR}"
    )

    find_library(ANDROID_LIB android)
    find_library(LOG_LIB     log)
    find_library(EGL_LIB     EGL)
    find_library(GLES_LIB    GLESv3)
endif()

# ======================= FMOD Vendor  =========================

add_library(FMOD::core SHARED IMPORTED GLOBAL)

if (WIN32 AND NOT ANDROID)
  set(_FMOD_INC "${VENDOR_DIR}/FMOD/windows/api/core/inc")
  set(_FMOD_LIB "${VENDOR_DIR}/FMOD/windows/api/core/lib/x64")

  set_target_properties(FMOD::core PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES       "${_FMOD_INC}"

    # Debug -> logging build (L), other configs -> non-logging
    IMPORTED_IMPLIB_DEBUG               "${_FMOD_LIB}/fmodL_vc.lib"
    IMPORTED_LOCATION_DEBUG             "${_FMOD_LIB}/fmodL.dll"

    IMPORTED_IMPLIB_RELEASE             "${_FMOD_LIB}/fmod_vc.lib"
    IMPORTED_LOCATION_RELEASE           "${_FMOD_LIB}/fmod.dll"

    IMPORTED_IMPLIB_RELWITHDEBINFO      "${_FMOD_LIB}/fmod_vc.lib"
    IMPORTED_LOCATION_RELWITHDEBINFO    "${_FMOD_LIB}/fmod.dll"

    IMPORTED_IMPLIB_MINSIZEREL          "${_FMOD_LIB}/fmod_vc.lib"
    IMPORTED_LOCATION_MINSIZEREL        "${_FMOD_LIB}/fmod.dll"
  )

elseif(ANDROID)
  set(_FMOD_INC "${VENDOR_DIR}/FMOD/android/api/core/inc")
  set(_FMOD_LIB "${VENDOR_DIR}/FMOD/android/api/core/lib/${ANDROID_ABI}")

  set_target_properties(FMOD::core PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES       "${_FMOD_INC}"

    # Debug -> logging .so, others -> non-logging
    IMPORTED_LOCATION_DEBUG             "${_FMOD_LIB}/libfmodL.so"
    IMPORTED_LOCATION_RELEASE           "${_FMOD_LIB}/libfmod.so"
    IMPORTED_LOCATION_RELWITHDEBINFO    "${_FMOD_LIB}/libfmod.so"
    IMPORTED_LOCATION_MINSIZEREL        "${_FMOD_LIB}/libfmod.so"
  )

  # NDK system libs FMOD needs on Android
  target_link_libraries(FMOD::core INTERFACE log android)
endif()

# ======================= STB (Image Loading) Vendor  =========================

# STB is header-only, but we need to avoid symbol conflicts with assimp
add_library(stb INTERFACE)
target_include_directories(stb INTERFACE "${VENDOR_DIR}/stb")

# Important: Define STB_IMAGE_IMPLEMENTATION only in one compilation unit
# This will be handled in the AssetPipeline source files
target_compile_definitions(stb INTERFACE 
    STB_AVAILABLE=1
)

# ======================= Assimp Vendor  =========================
if (WIN32 AND NOT ANDROID)
    # Check if assimp exists as submodule or prebuilt
    if (EXISTS "${VENDOR_DIR}/assimp/CMakeLists.txt")
        # Build from source (recommended for asset pipeline)
        
        # Configure assimp options before adding subdirectory
        set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
        set(ASSIMP_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
        set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
        set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
        set(ASSIMP_BUILD_ZLIB ON CACHE BOOL "" FORCE)
        
        # Important: Disable assimp's embedded stb to avoid conflicts
        set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
        set(ASSIMP_BUILD_OBJ_IMPORTER ON CACHE BOOL "" FORCE)
        set(ASSIMP_BUILD_FBX_IMPORTER ON CACHE BOOL "" FORCE)
        set(ASSIMP_BUILD_GLTF_IMPORTER ON CACHE BOOL "" FORCE)
        set(ASSIMP_BUILD_PLY_IMPORTER ON CACHE BOOL "" FORCE)
        
        # Add assimp subdirectory
        add_subdirectory("${VENDOR_DIR}/assimp" "${CMAKE_BINARY_DIR}/vendor_assimp" EXCLUDE_FROM_ALL)
        
        # Set runtime library to match your project
        set_property(TARGET assimp PROPERTY
            MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
            
        # DON'T create alias - assimp already creates assimp::assimp for us!
        # The alias is automatically created by assimp's CMakeLists.txt
        
        message(STATUS "Assimp built from source - assimp::assimp target available")
        
    elseif (EXISTS "${VENDOR_DIR}/assimp/lib")
        # Use prebuilt libraries
        add_library(assimp STATIC IMPORTED GLOBAL)
        set_target_properties(assimp PROPERTIES
            IMPORTED_LOCATION_DEBUG     "${VENDOR_DIR}/assimp/lib/assimp-vc142-mtd.lib"
            IMPORTED_LOCATION_RELEASE   "${VENDOR_DIR}/assimp/lib/assimp-vc142-mt.lib"
            INTERFACE_INCLUDE_DIRECTORIES "${VENDOR_DIR}/assimp/include"
        )
        
        # Only create alias for prebuilt version
        add_library(assimp::assimp ALIAS assimp)
        message(STATUS "Assimp using prebuilt libraries - assimp::assimp alias created")
        
    else()
        message(STATUS "Assimp not found - 3D model import will be disabled")
    endif()
endif()
