cmake_minimum_required(VERSION 3.22)
project(LuaBridgeSmoke LANGUAGES CXX)

set(SRC "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")

# This test needs sol2 headers and the lua::lua target provided by VendorTargets.cmake
if(NOT TARGET lua::lua)
  message(FATAL_ERROR "lua::lua target not found. Ensure VendorTargets.cmake adds Lua before this test.")
endif()
if(NOT TARGET sol2)
  message(FATAL_ERROR "sol2 target not found. Ensure VendorTargets.cmake adds sol2 before this test.")
endif()

if(ANDROID)
  # On Android we can't run a separate console exe easily.
  # Build the test as an OBJECT lib and inject it into PAINEngine,
  # autorunning via a static initializer.
  add_library(LuaBridgeSmoke_obj OBJECT ${SRC})
  target_compile_definitions(LuaBridgeSmoke_obj PRIVATE LUA_BRIDGE_SMOKE_AUTORUN=1)
  target_link_libraries(LuaBridgeSmoke_obj PRIVATE lua::lua sol2)

  # Native glue/log targets are defined in VendorTargets.cmake
  if(TARGET native_app_glue)
    target_link_libraries(LuaBridgeSmoke_obj PRIVATE native_app_glue)
  endif()
  if(DEFINED LOG_LIB)
    target_link_libraries(LuaBridgeSmoke_obj PRIVATE ${LOG_LIB})
  endif()

  # Inject into PAINEngine so it runs on startup
  if(TARGET PAINEngine)
    target_sources(PAINEngine PRIVATE $<TARGET_OBJECTS:LuaBridgeSmoke_obj>)
    target_link_libraries(PAINEngine PRIVATE lua::lua sol2)
  else()
    message(WARNING "PAINEngine target not found; LuaBridgeSmoke_obj not injected.")
  endif()

else()
  # Desktop: simple console exe you can run directly
  add_executable(LuaBridgeSmoke ${SRC})
  target_link_libraries(LuaBridgeSmoke PRIVATE lua::lua sol2)
  set_target_properties(LuaBridgeSmoke PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/tests"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/tests"
  )
  
  # --- Copy runtime DLLs (esp. lua54[d].dll) next to the test exe on Windows ---
if(WIN32)
  # Copy the lua DLL explicitly
  add_custom_command(TARGET LuaBridgeSmoke POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            $<TARGET_FILE:lua::lua>
            $<TARGET_FILE_DIR:LuaBridgeSmoke>)

  # (Optional) also copy any other dependent DLLs automatically (CMake 3.21+)
  add_custom_command(TARGET LuaBridgeSmoke POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            $<TARGET_RUNTIME_DLLS:LuaBridgeSmoke>
            $<TARGET_FILE_DIR:LuaBridgeSmoke>
    COMMAND_EXPAND_LISTS)
endif()


endif()
