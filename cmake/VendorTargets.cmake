# ======================= Header-Only Wrappers (if needed) =========================
if(NOT TARGET glm)
    add_library(glm INTERFACE)
    target_include_directories(glm INTERFACE "${glm_SOURCE_DIR}")
endif()

if(NOT TARGET nlohmann_json)
    add_library(nlohmann_json INTERFACE)
    target_include_directories(nlohmann_json INTERFACE "${nlohmann_json_SOURCE_DIR}/include")
endif()

if(NOT TARGET reflcpp)
    add_library(reflcpp INTERFACE)
    target_include_directories(reflcpp INTERFACE "${reflcpp_SOURCE_DIR}/include")
endif()

if(NOT TARGET spdlog_header_only)
    add_library(spdlog_header_only INTERFACE)
    target_include_directories(spdlog_header_only INTERFACE "${spdlog_SOURCE_DIR}/include")
endif()

if(NOT TARGET FileWatch_header_only)
    add_library(FileWatch_header_only INTERFACE)
    target_include_directories(FileWatch_header_only INTERFACE "${FileWatch_SOURCE_DIR}")
endif()

if(NOT TARGET gli_headers)
    add_library(gli_headers INTERFACE)
    target_include_directories(gli_headers INTERFACE "${gli_SOURCE_DIR}")
    target_link_libraries(gli_headers INTERFACE glm)
endif()

if(NOT TARGET entt_header_only)
    add_library(entt_header_only INTERFACE)
    target_include_directories(entt_header_only INTERFACE "${entt_SOURCE_DIR}/src")
endif()

if(NOT TARGET sol2)
    add_library(sol2 INTERFACE)
    target_include_directories(sol2 INTERFACE "${sol2_SOURCE_DIR}/include")
endif()

# ======================= GLEW (Windows) =========================
if (WIN32 AND NOT ANDROID)
# Assume FetchContent has already run and CMake has configured GLEW's targets correctly
if (TARGET libglew)
    # Use the provided target
    add_library(GLEW::GLEW ALIAS libglew)
elseif(TARGET libglew_static)
    add_library(GLEW::GLEW ALIAS libglew_static)
else()
    message(FATAL_ERROR "GLEW target not found! Check FetchContent_GLEW and its CMakeLists.txt.")
endif()
endif()

# ======================= ImGui =========================
if(NOT TARGET imgui::imgui)
    add_library(imgui STATIC
      "${imgui_SOURCE_DIR}/imgui.cpp"
      "${imgui_SOURCE_DIR}/imgui_draw.cpp"
      "${imgui_SOURCE_DIR}/imgui_tables.cpp"
      "${imgui_SOURCE_DIR}/imgui_widgets.cpp"
      "${imgui_SOURCE_DIR}/imgui_demo.cpp"
    )
    target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR})
    add_library(imgui::imgui ALIAS imgui)
endif()

set(IMGUI_DIR ${imgui_SOURCE_DIR} CACHE PATH "Path to ImGui sources")

# ======================= ImGuizmo =========================
if(NOT TARGET imguizmo)
    add_library(imguizmo STATIC
        "${imguizmo_SOURCE_DIR}/ImGuizmo.cpp"
        "${imguizmo_SOURCE_DIR}/ImGuizmo.h"
    )
    # Include both public interface dir and private compilation includes
    target_include_directories(imguizmo 
        PUBLIC 
            ${imguizmo_SOURCE_DIR}
        PRIVATE
            ${imgui_SOURCE_DIR}
    )
    target_link_libraries(imguizmo PUBLIC imgui::imgui)
    target_compile_definitions(imguizmo PRIVATE IMGUI_DEFINE_MATH_OPERATORS)
endif()

# ======================= stb =========================
if (WIN32 AND NOT ANDROID)
    add_library(stb_implementation STATIC "${CMAKE_CURRENT_LIST_DIR}/stb_impl.cpp")
    target_include_directories(stb_implementation PUBLIC "${stb_SOURCE_DIR}")

    add_library(stb INTERFACE)
    target_include_directories(stb INTERFACE "${stb_SOURCE_DIR}")
    target_link_libraries(stb INTERFACE stb_implementation)
endif()

# ======================= Platform-specific/GL/Android/NDK glue =========================
if (WIN32 AND NOT ANDROID)
    find_package(OpenGL REQUIRED)
endif()

if (ANDROID)
    set(_NDK "${CMAKE_ANDROID_NDK}")
    if(NOT _NDK AND DEFINED ANDROID_NDK)
        set(_NDK "${ANDROID_NDK}")
    endif()
    if(NOT _NDK)
        message(FATAL_ERROR "Cannot find NDK path (CMAKE_ANDROID_NDK/ANDROID_NDK not set)")
    endif()
    set(NATIVE_APP_GLUE_DIR "${_NDK}/sources/android/native_app_glue")
    add_library(native_app_glue STATIC "${NATIVE_APP_GLUE_DIR}/android_native_app_glue.c")
    target_include_directories(native_app_glue PUBLIC "${NATIVE_APP_GLUE_DIR}")
    find_library(ANDROID_LIB android)
    find_library(LOG_LIB     log)
    find_library(EGL_LIB     EGL)
    find_library(GLES_LIB    GLESv3)
endif()

# ======================= FMOD =========================
add_library(FMOD::core SHARED IMPORTED GLOBAL)
set(VENDOR_DIR "${CMAKE_SOURCE_DIR}/vendor")
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

# ======================= Cuttlefish Tool Wrapper =========================
if(WIN32 AND NOT ANDROID)
    find_program(CUTTLEFISH_EXECUTABLE
        NAMES cuttlefish cuttlefish.exe
        PATHS "${cuttlefish_SOURCE_DIR}/bin" "${cuttlefish_SOURCE_DIR}"
        ENV PATH
        DOC "Cuttlefish texture compression tool"
    )
    if(CUTTLEFISH_EXECUTABLE)
        add_library(Cuttlefish::Cuttlefish INTERFACE IMPORTED GLOBAL)
        set_target_properties(Cuttlefish::Cuttlefish PROPERTIES INTERFACE_COMPILE_DEFINITIONS "CUTTLEFISH_EXECUTABLE=\"${CUTTLEFISH_EXECUTABLE}\"")
        message(STATUS "Cuttlefish found: ${CUTTLEFISH_EXECUTABLE}")
    else()
        message(STATUS "Cuttlefish not found - BC/ASTC compression disabled")
    endif()
endif()

# ======================= Assimp, GLFW, Jolt: Use Official Targets =========================
# No manual add_library needed; just link to assimp::assimp, glfw, Jolt, etc.
if(NOT TARGET Jolt)
    add_library(Jolt INTERFACE)
    target_include_directories(Jolt INTERFACE "${jolt_SOURCE_DIR}")
endif()

message(STATUS "All vendor targets configured for FetchContent system")

# ======================= FreeType =========================
if(NOT TARGET freetype)
    # FreeType builds itself via its own CMakeLists.txt
    # This assumes freetype is fetched via FetchContent in ImportDependencies.cmake
    # If not, you need to add it there first
    message(STATUS "FreeType configured from: ${freetype_SOURCE_DIR}")
endif()

message(STATUS "All vendor targets configured for FetchContent system")
