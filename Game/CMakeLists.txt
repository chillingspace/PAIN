# Define the executable target for the Game
add_executable(Game src/Game.cpp)

# --- Link against the PAINEngine ---
# This is the key step. By linking to PAINEngine, the "Game" target
# automatically inherits all the PUBLIC include directories and linked libraries
# from PAINEngine (like spdlog, glm, glfw, etc.).
target_link_libraries(Game PRIVATE PAINEngine)

# --- Post-Build Command for Windows ---
# This ensures that assets are copied to the output directory for easy debugging.
if(NOT ANDROID)
    add_custom_command(TARGET Game POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/game-assets"
        "$<TARGET_FILE_DIR:Game>/assets"
        COMMENT "Copying game assets..."
    )
    # This sets the working directory in Visual Studio so the app can find the assets.
    set_target_properties(Game PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:Game>")
endif()