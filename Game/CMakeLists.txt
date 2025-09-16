# Collect all game source files
file(GLOB_RECURSE GAME_SRC CONFIGURE_DEPENDS src/*.cpp src/*.h)

# Build sandbox as an executable
add_executable(Game ${GAME_SRC})

# Link against the engine
target_link_libraries(Game PRIVATE PAINEngine)

# Include engine headers
target_include_directories(Game PRIVATE ${CMAKE_SOURCE_DIR}/PAINEngine/src)

# --- Post-Build Commands (PC Only) ---
if(NOT ANDROID)
    # This command copies the required DLLs next to your Game.exe after it's built
    add_custom_command(TARGET Game POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/PAINEngine/vendor/FMOD/windows/api/core/lib/x64/fmod.dll"
            "$<TARGET_FILE_DIR:Game>"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/PAINEngine/vendor/FMOD/windows/api/core/lib/x64/fmodL.dll"
            "$<TARGET_FILE_DIR:Game>"
        COMMENT "Copying runtime DLLs to output directory..."
    )

    # This command copies your game assets to the build output directory
    add_custom_command(TARGET Game POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
            "${CMAKE_SOURCE_DIR}/assets"
            "$<TARGET_FILE_DIR:Game>/assets"
        COMMENT "Copying game assets..."
    )
endif()

# --- Visual Studio Specific Properties ---
# Set the debugging working directory to the output directory (e.g., bin/Debug)
set_target_properties(Game PROPERTIES
    VS_DEBUGGING_WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
)