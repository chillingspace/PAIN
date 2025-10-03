cmake_minimum_required(VERSION 3.22)

message(STATUS "=== Building PAINAssetTypes ===")

# ======================= AssetTypes Library  =========================
# Basic asset types (GUID, AssetType, AssetDescriptor) - Header-only interface library

# Collect all header files for IDE visibility FROM SRC FOLDER (not include)
file(GLOB_RECURSE ASSET_TYPES_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp"
)

message(STATUS "Found AssetTypes headers: ${ASSET_TYPES_HEADERS}")

# Create interface library for header-only types
add_library(PAINAssetTypes INTERFACE)

# Add header files to target for IDE support (CMake 3.19+)
if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.19")
    target_sources(PAINAssetTypes INTERFACE ${ASSET_TYPES_HEADERS})
endif()

# Set include directories - Point to SRC folder where headers actually are
target_include_directories(PAINAssetTypes
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:include>
)

# Set C++ standard for consuming targets
target_compile_features(PAINAssetTypes INTERFACE cxx_std_17)

# Link basic vendor dependencies that AssetTypes needs
if(TARGET nlohmann_json)
    target_link_libraries(PAINAssetTypes INTERFACE nlohmann_json)
endif()

# Optional: Add compile definitions
target_compile_definitions(PAINAssetTypes INTERFACE
    PAIN_ASSET_TYPES_VERSION_MAJOR=1
    PAIN_ASSET_TYPES_VERSION_MINOR=0
)

message(STATUS "PAINAssetTypes configured with src/ directory")
