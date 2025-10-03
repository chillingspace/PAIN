# ======================= Project Configuration =========================
# Cmake version
cmake_minimum_required(VERSION 3.22)

# Main Project
project(PAIN)

# Set Cmake Standards
if(ANDROID)
	set(CMAKE_CXX_STANDARD 17)
else()
	set(CMAKE_CXX_STANDARD 20)
	set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

# Set the directory for our custom CMake modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Include our dependency helper module
include(ImportDependencies)
importDependencies() # Placeholder

# Global compile flags and macros
if (ANDROID)
  add_compile_definitions(PLATFORM_ANDROID)
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION OFF)
endif()

# Use /MD in Release and /MDd in Debug for all MSVC targets
if (MSVC)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()

# ======================= Vendors  =========================

include(VendorTargets)   # defines imgui, glfw, glm, nlohmann_json, spdlog_header_only, Jolt::Jolt, etc.

# ======================= Output Directories  =========================

if (NOT ANDROID)
	# Keep your bin/bin-int structure
	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin-int/${CMAKE_BUILD_TYPE})
	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE})
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE})

	foreach(cfg Debug Release RelWithDebInfo MinSizeRel)
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

# --- Add Subprojects ---
add_subdirectory(PAINEngine)
add_subdirectory(Game)

if (ANDROID)
  add_subdirectory(android/app/src/main/cpp)
endif()

# Set the startup project for the Visual Studio solution
if (MSVC)
  set_property(DIRECTORY . PROPERTY VS_STARTUP_PROJECT Game)
endif()