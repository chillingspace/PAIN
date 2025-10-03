cmake_minimum_required(VERSION 3.22)

# Asset Pipeline source files
file(GLOB_RECURSE PIPELINE_SRC CONFIGURE_DEPENDS 
    src/*.cpp 
    src/*.h
)

# Remove any main.cpp files from library sources
list(FILTER PIPELINE_SRC EXCLUDE REGEX ".*main\\.cpp$")

# Asset Pipeline Library - Only for host platforms (build-time tools)
add_library(PAINAssetPipeline STATIC ${PIPELINE_SRC})

# Core dependencies
target_link_libraries(PAINAssetPipeline
    PUBLIC 
        PAINAssetTypes        # Basic asset types
        nlohmann_json         # JSON for descriptors
    PRIVATE
        spdlog_header_only    # Logging
)

# Asset processing dependencies - Only if available
if(TARGET assimp::assimp AND NOT ANDROID)
    target_link_libraries(PAINAssetPipeline PRIVATE assimp::assimp)
    target_compile_definitions(PAINAssetPipeline PRIVATE 
        HAS_ASSIMP=1
        PAIN_ENABLE_3D_MODELS=1
    )
    message(STATUS "AssetPipeline: assimp enabled - 3D model import available")
endif()

if(TARGET stb AND NOT ANDROID)
    target_link_libraries(PAINAssetPipeline PRIVATE stb)
    target_compile_definitions(PAINAssetPipeline PRIVATE 
        HAS_STB_IMAGE=1
        PAIN_ENABLE_IMAGE_LOADING=1
    )
    message(STATUS "AssetPipeline: stb_image enabled - Image loading available")
endif()

# Include directories
target_include_directories(PAINAssetPipeline
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:include>
)

# C++ standard
target_compile_features(PAINAssetPipeline PUBLIC cxx_std_17)

# Platform-specific compile definitions
target_compile_definitions(PAINAssetPipeline
    PRIVATE
        $<$<CONFIG:Debug>:PAIN_ASSET_DEBUG=1>
        $<$<PLATFORM_ID:Windows>:PAIN_WINDOWS=1>
        PAIN_ASSET_PIPELINE_ENABLED=1
)
