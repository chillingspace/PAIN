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
  add_executable(Ribbit_Express ${GAME_SRC})
  
  # Configure Subsystem: Console for Debug, Windows (GUI) for Release
  # This matches EntryPoint.h which defines main() for Debug and WinMain() for Release
  target_link_options(Ribbit_Express PRIVATE 
      $<$<CONFIG:Debug>:/SUBSYSTEM:CONSOLE>
      $<$<NOT:$<CONFIG:Debug>>:/SUBSYSTEM:WINDOWS>
  )
  target_link_libraries(Ribbit_Express PRIVATE PAINEngine)
  add_dependencies(Ribbit_Express CompileAllAssets)

  # Copy the correct FMOD runtime next to Game.exe using the imported target
  # Debug -> fmodL.(dll), others -> fmod.(dll)
  add_custom_command(TARGET Ribbit_Express POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:lua>"
            "$<TARGET_FILE:FMOD::core>"
            "$<TARGET_FILE_DIR:Ribbit_Express>"
    COMMENT "Copying FMOD runtime DLL...")

   add_custom_command(TARGET Ribbit_Express POST_BUILD
     COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/build/Tools/cuttlefish"
            "$<TARGET_FILE_DIR:Ribbit_Express>/cuttlefish"
     COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/build/Tools/ffmpeg"
            "$<TARGET_FILE_DIR:Ribbit_Express>/ffmpeg"
     COMMENT "Copying cuttlefish, astc, ffmpeg to debug output dir"
     CONFIGURATIONS Debug
   )

  # VS debugger working directory
  set_target_properties(Ribbit_Express PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:Ribbit_Express>")
endif()

message(STATUS "Game configuration successful.")
