set(VENDOR_DIR "${CMAKE_SOURCE_DIR}/vendor")

# ------------------------
# Header-only libraries
# ------------------------
add_library(glm INTERFACE)
target_include_directories(glm INTERFACE "${VENDOR_DIR}/glm")

add_library(nlohmann_json INTERFACE)
# If you have the single-header json.hpp under vendor/nlohmann/include
#   include dir should be the parent containing nlohmann/json.hpp
target_include_directories(nlohmann_json INTERFACE "${VENDOR_DIR}/nlohmann/include")

add_library(spdlog_header_only INTERFACE)
target_include_directories(spdlog_header_only INTERFACE "${VENDOR_DIR}/spdlog/include")

# GL headers convenience (optional, if you keep <GL/*.h> here)
add_library(gl_headers INTERFACE)
target_include_directories(gl_headers INTERFACE "${VENDOR_DIR}/GL")

# ------------------------
# Dear ImGui (core files)
# ------------------------
add_library(imgui STATIC
  "${VENDOR_DIR}/ImGui/imgui.cpp"
  "${VENDOR_DIR}/ImGui/imgui_draw.cpp"
  "${VENDOR_DIR}/ImGui/imgui_tables.cpp"
  "${VENDOR_DIR}/ImGui/imgui_widgets.cpp"
)
target_include_directories(imgui PUBLIC "${VENDOR_DIR}/ImGui")
add_library(imgui::imgui ALIAS imgui)

# Expose IMGUI dir path to subprojects for backends
set(IMGUI_DIR "${VENDOR_DIR}/ImGui" CACHE PATH "Path to ImGui sources")

# ------------------------
# GLFW
# ------------------------
# Two modes:
#  A) Build from source if vendor/GLFW contains the official CMakeLists.txt
#  B) Otherwise, treat as prebuilt (IMPORTED) and point to your .lib/.a

if (EXISTS "${VENDOR_DIR}/GLFW/CMakeLists.txt")
  # A) From source
  set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
  add_subdirectory("${VENDOR_DIR}/GLFW" "${CMAKE_BINARY_DIR}/vendor_glfw")
  # This creates the target "glfw"
else()
  # B) Prebuilt import (Windows example)
  add_library(glfw STATIC IMPORTED GLOBAL)
  # TODO: Change IMPORTED_LOCATION to your actual prebuilt lib file
  # e.g., vendor/GLFW/lib-vc2022/glfw3.lib or lib/win64/glfw3.lib
  set_target_properties(glfw PROPERTIES
    IMPORTED_LOCATION             "${VENDOR_DIR}/GLFW/lib/glfw3.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${VENDOR_DIR}/GLFW/include"
  )
endif()

# ------------------------
# Jolt Physics
# ------------------------
# (Optional but recommended) choose Jolt options BEFORE add_subdirectory.
# They’ll become the default values in the Jolt subproject cache.
# See docs for meaning of these flags. :contentReference[oaicite:1]{index=1}
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)               # build static lib
set(CPP_RTTI_ENABLED OFF CACHE BOOL "" FORCE)                # Jolt default: no RTTI
# set(JPH_USE_STD_VECTOR OFF CACHE BOOL "" FORCE)            # keep Jolt's Array by default
# set(DEBUG_RENDERER_IN_DEBUG_AND_RELEASE ON CACHE BOOL "" FORCE) # if you want debug draw in Debug/Release
# set(DEBUG_RENDERER_IN_DISTRIBUTION OFF CACHE BOOL "" FORCE)

# Add the Jolt project (use EXCLUDE_FROM_ALL to avoid building Samples etc. unless asked)
add_subdirectory("${VENDOR_DIR}/Jolt" "${CMAKE_BINARY_DIR}/vendor_jolt" EXCLUDE_FROM_ALL)

# ------------------------
# System GL (platform-specific)
# ------------------------
if (WIN32 AND NOT ANDROID)
  find_package(OpenGL REQUIRED) # provides OpenGL::GL
endif()

if (ANDROID)
    find_library(ANDROID_LIB android)
    find_library(LOG_LIB     log)
    find_library(EGL_LIB     EGL)
    find_library(GLES_LIB    GLESv3)  # or GLESv2 if you target ES 2.0
endif()

# ------------------------
# FMOD
# ------------------------
if(ANDROID)
    # For Android, paths are handled by the android/app/src/main/cpp/CMakeLists.txt but define the library names for consistency
    set(FMOD_LIBS fmod fmodL)
else()
    # For PC, set include and library paths
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/vendor/FMOD/windows/api/core/inc)
    link_directories(${CMAKE_CURRENT_SOURCE_DIR}/vendor/FMOD/windows/api/core/lib/x64)
    set(FMOD_LIBS fmod_vc fmodL_vc)
endif()