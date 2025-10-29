cmake_minimum_required(VERSION 3.22)

message(STATUS "=== Building PAINAssetPipeline ===")

# ======================= Platform Detection & Definitions =========================

# Detect target platform and set custom definitions
if(WIN32)
    message(STATUS "AssetPipeline: Building for Windows")
elseif(ANDROID)
    message(STATUS "AssetPipeline: Building for Android")
elseif(UNIX AND NOT APPLE)
    message(STATUS "AssetPipeline: Building for Linux")
elseif(APPLE)
    message(STATUS "AssetPipeline: Building for macOS")
endif()

# ======================= Asset Output Path Configuration =========================

# Configure asset output paths based on platform
if(WIN32)
    set(PAIN_ASSETS_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/bin")
elseif(ANDROID)
    # Android: Ship to app/src/main/assets/ (gets packaged into APK)
    set(PAIN_ASSETS_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/app/src/main/assets")
endif()

message(STATUS "AssetPipeline output directory: ${PAIN_ASSETS_OUTPUT_DIR}")

# ======================= Source Files =========================

# Asset Pipeline source files
file(GLOB_RECURSE PIPELINE_SRC CONFIGURE_DEPENDS 
    src/*.cpp 
    src/*.h
)

# Remove any main.cpp files from library sources
list(FILTER PIPELINE_SRC EXCLUDE REGEX ".*main\\.cpp$")

# Asset Pipeline Library
add_library(PAINAssetPipeline STATIC ${PIPELINE_SRC})

# ======================= Core Dependencies =========================

# Core dependencies - PAINAssetTypes MUST be linked first
target_link_libraries(PAINAssetPipeline
    PUBLIC 
        PAINAssetTypes        # Basic asset types
)

# Add vendor dependencies
if(TARGET nlohmann_json)
    target_link_libraries(PAINAssetPipeline PUBLIC nlohmann_json)
endif()

if(TARGET spdlog_header_only)
    target_link_libraries(PAINAssetPipeline PRIVATE spdlog_header_only)
endif()


# ======================= Platform-Specific Dependencies =========================

# Assimp (transitive) — only on host (not Android), and only if the target exists
if(NOT ANDROID)
  if(TARGET assimp::assimp)
    target_link_libraries(PAINAssetPipeline
      PUBLIC
        assimp::assimp
    )
    target_compile_definitions(PAINAssetPipeline PRIVATE
      HAS_ASSIMP=1
      PAIN_ENABLE_3D_MODELS=1
    )
    message(STATUS "AssetPipeline: Assimp enabled - 3D model import available")
  else()
    message(WARNING "AssetPipeline: assimp::assimp target not found; 3D model import disabled")
  endif()
endif()


# Asset processing dependencies - Only for host platforms (not Android runtime)
if(TARGET stb AND NOT ANDROID)
    target_link_libraries(PAINAssetPipeline PRIVATE stb)
    target_compile_definitions(PAINAssetPipeline PRIVATE 
        HAS_STB_IMAGE=1
        PAIN_ENABLE_IMAGE_LOADING=1
    )
    message(STATUS "AssetPipeline: STB enabled - Image loading available")
endif()

if(TARGET Cuttlefish::Cuttlefish AND WIN32)
    target_link_libraries(PAINAssetPipeline PRIVATE Cuttlefish::Cuttlefish)
    target_compile_definitions(PAINAssetPipeline PRIVATE 
        HAS_CUTTLEFISH=1
        PAIN_ENABLE_BC_COMPRESSION=1
    )
    message(STATUS "AssetPipeline: Cuttlefish enabled - BC compression available")
endif()

if(TARGET ASTC::Encoder AND NOT ANDROID)
    target_link_libraries(PAINAssetPipeline PRIVATE ASTC::Encoder)
    target_compile_definitions(PAINAssetPipeline PRIVATE 
        HAS_ASTC_ENCODER=1
        PAIN_ENABLE_ASTC_COMPRESSION=1
    )
    message(STATUS "AssetPipeline: ASTC Encoder enabled - ASTC compression available")
endif()

# ======================= Compile Definitions =========================

# Platform-specific definitions (your requested PN_PLATFORM_* macros)
target_compile_definitions(PAINAssetPipeline
    PRIVATE
        # Custom platform macros
        $<$<BOOL:${WIN32}>:PN_PLATFORM_WINDOWS=1>
        $<$<BOOL:${ANDROID}>:PN_PLATFORM_ANDROID=1>
        
        # Asset output directory
        PAIN_ASSETS_OUTPUT_DIR="${PAIN_ASSETS_OUTPUT_DIR}"
        
        # Build configuration
        $<$<CONFIG:Debug>:PAIN_ASSET_DEBUG=1>
        $<$<CONFIG:Release>:PAIN_ASSET_RELEASE=1>
        
        # General flags
        PAIN_ASSET_PIPELINE_ENABLED=1
)

# ======================= Include Directories =========================

# Include directories for AssetPipeline itself
target_include_directories(PAINAssetPipeline
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:include>
)

# ======================= Compiler Features =========================

# C++ standard
target_compile_features(PAINAssetPipeline PUBLIC cxx_std_17)

message(STATUS "PAINAssetPipeline configured successfully")



