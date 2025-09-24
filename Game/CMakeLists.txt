# Collect all game source files
file(GLOB_RECURSE GAME_SRC CONFIGURE_DEPENDS src/*.cpp src/*.h)

# Build style: desktop exe, Android static lib
if (ANDROID)
  add_library(GameCore STATIC ${GAME_SRC})
  target_link_libraries(GameCore PUBLIC PAINEngine)

else()
  add_executable(Game ${GAME_SRC})
  target_link_libraries(Game PRIVATE PAINEngine)

  # Copy the correct FMOD runtime next to Game.exe using the imported target
  # Debug -> fmodL.(dll), others -> fmod.(dll)
  add_custom_command(TARGET Game POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:FMOD::core>"
            "$<TARGET_FILE_DIR:Game>"
    COMMENT "Copying FMOD runtime DLL...")

  # Copy assets
  add_custom_command(TARGET Game POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/assets"
            "$<TARGET_FILE_DIR:Game>/assets"
    COMMENT "Copying game assets...")

  # VS debugger working directory
  set_target_properties(Game PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:Game>")
endif()
