cmake_minimum_required(VERSION 3.22)
message(STATUS "Game configuration begin.")

# Collect all game source files
file(GLOB_RECURSE GAME_SRC CONFIGURE_DEPENDS src/*.cpp src/*.h)

# Build style: desktop exe, Android static lib
if (ANDROID)
  add_library(GameCore STATIC ${GAME_SRC})
  target_link_libraries(GameCore PUBLIC PAINEngine)
  add_dependencies(GameCore CompileAllAssets)

else()
  add_executable(Friends-Frog-ever ${GAME_SRC})
  
  # Configure Subsystem: Console for Debug, Windows (GUI) for Release
  # This matches EntryPoint.h which defines main() for Debug and WinMain() for Release
  target_link_options(Friends-Frog-ever PRIVATE 
      $<$<CONFIG:Debug>:/SUBSYSTEM:CONSOLE>
      $<$<NOT:$<CONFIG:Debug>>:/SUBSYSTEM:WINDOWS>
  )
  target_link_libraries(Friends-Frog-ever PRIVATE PAINEngine)
  add_dependencies(Friends-Frog-ever CompileAllAssets)

  # Copy the correct FMOD runtime next to Game.exe using the imported target
  # Debug -> fmodL.(dll), others -> fmod.(dll)
  add_custom_command(TARGET Friends-Frog-ever POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:lua>"
            "$<TARGET_FILE:FMOD::core>"
            "$<TARGET_FILE_DIR:Friends-Frog-ever>"
    COMMENT "Copying FMOD runtime DLL...")

  add_custom_command(TARGET Friends-Frog-ever POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E $<IF:$<CONFIG:Debug>,copy_directory,true>
            "${CMAKE_SOURCE_DIR}/build/Tools/cuttlefish"
            "$<TARGET_FILE_DIR:Friends-Frog-ever>/cuttlefish"
    COMMAND ${CMAKE_COMMAND} -E $<IF:$<CONFIG:Debug>,copy_directory,true>
            "${CMAKE_SOURCE_DIR}/build/Tools/ffmpeg"
            "$<TARGET_FILE_DIR:Friends-Frog-ever>/ffmpeg"
    COMMENT "Copying cuttlefish, astc, ffmpeg to debug output dir"
    VERBATIM
  )

  # VS debugger working directory
  set_target_properties(Friends-Frog-ever PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:Friends-Frog-ever>")
endif()

message(STATUS "Game configuration successful.")
