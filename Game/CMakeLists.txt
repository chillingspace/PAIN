# Collect all game source files
file(GLOB_RECURSE GAME_SRC CONFIGURE_DEPENDS "src/*.cpp")

# Build the game as an executable
add_executable(Game ${GAME_SRC})

# Link against the engine
target_link_libraries(Game PRIVATE PAINEngine)

# --- Post-Build Steps (for Desktop builds) ---
if(NOT ANDROID)
    # Copy FMOD runtime DLLs next to the game executable
    add_custom_command(TARGET Game POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:fmod>
            $<TARGET_FILE_DIR:Game>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:fmodL>
            $<TARGET_FILE_DIR:Game>
        COMMENT "Copying runtime DLLs to output directory..."
    )

    # Copy your game assets to the build output directory
    add_custom_command(TARGET Game POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
            "${CMAKE_SOURCE_DIR}/game-assets"
            "$<TARGET_FILE_DIR:Game>/game-assets"
        COMMENT "Copying game assets..."
    )
endif()

# --- Visual Studio Specific Properties ---
if(MSVC)
    # Set the debugging working directory to the output directory
    set_target_properties(Game PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:Game>"
    )
endif()