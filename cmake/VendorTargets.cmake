set(VENDOR_DIR "${CMAKE_SOURCE_DIR}/vendor")

# ======================= Header Only Vendors  =========================
add_library(glm INTERFACE)
target_include_directories(glm INTERFACE "${VENDOR_DIR}/glm")

add_library(nlohmann_json INTERFACE)
target_include_directories(nlohmann_json INTERFACE "${VENDOR_DIR}/nlohmann/include")

add_library(spdlog_header_only INTERFACE)
target_include_directories(spdlog_header_only INTERFACE "${VENDOR_DIR}/spdlog/include")

add_library(gl_headers INTERFACE)
target_include_directories(gl_headers INTERFACE "${VENDOR_DIR}/GL")

# ======================= ImGui Vendor  =========================

set(_GLEW_DIR "${CMAKE_SOURCE_DIR}/vendor/glew")

add_library(_glew STATIC
  "${_GLEW_DIR}/src/glew.c"
)
target_include_directories(_glew PUBLIC "${_GLEW_DIR}/include")
target_compile_definitions(_glew PUBLIC GLEW_STATIC)  # ensure headers use static path

add_library(GLEW::GLEW ALIAS _glew)

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

# ======================= Jolt Vendor  =========================

# (Optional but recommended) choose Jolt options BEFORE add_subdirectory.
# They’ll become the default values in the Jolt subproject cache.
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