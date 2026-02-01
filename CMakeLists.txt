# ======================= Project Configuration =========================
# Cmake version
cmake_minimum_required(VERSION 3.22)

# Main Project
project(PAIN)

# Set Cmake Standards
if(ANDROID)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DJPH_DISABLE_ASSERTS -DNDEBUG")
else()
	set(CMAKE_CXX_STANDARD 20)
	set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

# Set the directory for our custom CMake modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_program(GIT_EXECUTABLE git)
if(GIT_EXECUTABLE)
    message(STATUS "Git found at: ${GIT_EXECUTABLE}")
else()
    message(FATAL_ERROR "Git not found in PATH!")
endif()

# Use /MD in Release and /MDd in Debug for all MSVC targets
if (MSVC)
  add_compile_options(/MP)
  add_compile_options(/bigobj)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# Include our dependency helper module
include(ImportDependencies)
importDependencies() # Placeholder

# Global compile flags and macros
if (ANDROID)
  add_compile_definitions(PLATFORM_ANDROID)
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION OFF)
	
add_compile_definitions(
    JPH_DISABLE_ASSERTS
    JPH_PROFILE_ENABLED=1
    JPH_DEBUG_RENDERER=0
    NDEBUG
  )

  # Target a compatible instruction set for x86_64 emulators to prevent SIGILL crashes
  if(CMAKE_ANDROID_ARCH_ABI STREQUAL "x86_64")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=westmere")
  endif()
endif()



# ======================= Vendors  =========================

include(VendorTargets)   # defines imgui, glfw, glm, nlohmann_json, spdlog_header_only, Jolt::Jolt, etc.

# ======================= Output Directories  =========================

if (NOT ANDROID)
	# Keep your bin/bin-int structure
	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin-int/${CMAKE_BUILD_TYPE})
	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE})
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE})

	foreach(cfg Debug Release)
	  string(TOUPPER "${cfg}" CFG)
	  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CFG} "${CMAKE_SOURCE_DIR}/bin/${cfg}")
	  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CFG} "${CMAKE_SOURCE_DIR}/bin/${cfg}")
	  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${CFG} "${CMAKE_SOURCE_DIR}/bin-int/${cfg}")
	endforeach()
endif()

# ======================= Output Directories  =========================

add_subdirectory(Common/AssetTypes)
add_subdirectory(Common/AssetPipeline)

if (NOT ANDROID)
add_subdirectory(Tools)
endif()

# Include asset compilation integration
include(AssetsCompiler)

if(TARGET cuttlefish)
    add_dependencies(AssetCompilerTool cuttlefish copy_cuttlefish_bin)
endif()

if(TARGET astcenc)
    add_dependencies(AssetCompilerTool astcenc-native copy_astcenc_bin)
endif()

if(NOT ANDROID)
	add_dependencies(AssetCompilerTool copy_ffmpeg_bin)
endif()

# --- Add Subprojects ---
add_subdirectory(PAINEngine)
#add_subdirectory(Tests/LuaBridgeSmoke)
add_subdirectory(Game)

if (ANDROID)
  add_subdirectory(android/app/src/main/cpp)
endif()

# Set the startup project for the Visual Studio solution
if (MSVC)
  set_property(DIRECTORY . PROPERTY VS_STARTUP_PROJECT Friends-Frog-ever)
  add_compile_options(/MP)
  add_compile_options(/bigobj)
endif()