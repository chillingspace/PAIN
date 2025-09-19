# ======================= Project Configuration =========================
# Using the exact versioning from the original working sample
cmake_minimum_required(VERSION 2.8...3.15)
cmake_policy(VERSION 3.5)

project(PAIN-CrossPlatform)

# --- vcpkg, Dependencies, and Helpers ---
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
set(VCPKG_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/libs/vcpkg")

include(vcpkg_windows)
importVcPkgDependencies()

include(ImportDependencies)
importDependencies()

find_package(imgui CONFIG REQUIRED)

# --- Global Definitions ---
if(NOT ANDROID)
    add_definitions(-DGLEW_STATIC)
else()
    add_definitions(-DPLATFORM_ANDROID)
endif()

# --- List of External Libraries (platform-specific) ---
if(ANDROID)
    set(ALL_LIBS
        PRIVATE imgui::imgui  # From vcpkg
        PRIVATE spdlog::spdlog
        PRIVATE Jolt
        # FMOD libs are added directly in the Android-specific CMakeLists.txt
    )
else()
    set(ALL_LIBS
        PRIVATE glfw
        PRIVATE glew_static
        PRIVATE imgui::imgui  # From vcpkg
        PRIVATE spdlog::spdlog
        PRIVATE Jolt
        PRIVATE "${CMAKE_SOURCE_DIR}/libs/fmod/lib/x64/fmod_vc.lib"
        PRIVATE "${CMAKE_SOURCE_DIR}/libs/fmod/lib/x64/fmodL_vc.lib"
        PRIVATE opengl32
    )
endif()

# --- FMOD Include Paths (same for both platforms) ---
if(ANDROID)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/android/app/src/main/cpp/libs/fmod/inc)
else()
    include_directories(${CMAKE_SOURCE_DIR}/libs/fmod/inc)
endif()

# --- Source File Configuration ---
# MODIFIED: Point to our new unified engine folder
set(SRC_DIR ./PAINEngine)

# MODIFIED: Find all .cpp files in the PAINEngine folder and its subdirectories
file(GLOB_RECURSE APP_SRC
    "${SRC_DIR}/*.cpp"
    "${SRC_DIR}/Applications/*.cpp"
    "${SRC_DIR}/Audio/*.cpp"
    "${SRC_DIR}/CoreSystems/Collision/*.cpp"
    "${SRC_DIR}/CoreSystems/Renderer/*.cpp"
    "${SRC_DIR}/CoreSystems/Windows/GLFW/*.cpp"
    "${SRC_DIR}/CoreSystems/Windows/OpenGL/*.cpp"
    "${SRC_DIR}/ECS/*.cpp"
    "${SRC_DIR}/ECS/System/*.cpp"
    "${SRC_DIR}/LayeredSystems/LevelEditor/*.cpp"
    "${SRC_DIR}/LayeredSystems/LevelEditor/Panels/*.cpp"
    "${SRC_DIR}/Logging/*.cpp"
    "${SRC_DIR}/Systems/Physics/*.cpp"
)

# --- Build the Executable ---
add_executable(${CMAKE_PROJECT_NAME} ${APP_SRC})

# --- Linking ---
target_link_libraries(${CMAKE_PROJECT_NAME} ${ALL_LIBS})

# Set C++ Standard
set_property(TARGET ${CMAKE_PROJECT_NAME} PROPERTY CXX_STANDARD 20)

# --- Include Directories ---
# This is the "Golden Rule" setting. It tells the compiler to always
# start looking for our headers from the PAINEngine folder.
target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC ${SRC_DIR})

# --- Post-Build Commands ---
# MODIFIED: Copy the correct assets folder and the new imgui.ini
if(NOT ANDROID)
    add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
            "${CMAKE_SOURCE_DIR}/assets"
            "$<TARGET_FILE_DIR:${CMAKE_PROJECT_NAME}>/assets"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/Game/imgui.ini"
            "$<TARGET_FILE_DIR:${CMAKE_PROJECT_NAME}>/imgui.ini"
        COMMENT "Copying game assets and imgui.ini..."
    )
endif()