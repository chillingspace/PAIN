# ======================= Project Configuration =========================
# Cmake version
cmake_minimum_required(VERSION 3.16)

# Main Project
project(PAIN)

# Set Cmake Standards
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set the directory for our custom CMake modules
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Include our dependency helper module
include(ImportDependencies)
importDependencies() # Placeholder

# Global compile flags and macros
if (ANDROID)
  add_compile_definitions(PLATFORM_ANDROID)
endif()

# ======================= Vendors  =========================

include(VendorTargets)   # defines imgui, glfw, glm, nlohmann_json, spdlog_header_only, Jolt::Jolt, etc.

# ======================= Output Directories  =========================

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

# --- Add Subprojects ---
add_subdirectory(PAINEngine)
add_subdirectory(Game)

# Set the startup project for the Visual Studio solution
set_property(DIRECTORY . PROPERTY VS_STARTUP_PROJECT Game)