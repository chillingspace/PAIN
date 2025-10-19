cmake_minimum_required(VERSION 3.21)
project(lua54 C)

# Sources from the official Lua distribution (no lua.c, luac.c)
set(LUA_CORE
  lapi.c lauxlib.c lbaselib.c lcode.c lcorolib.c lctype.c ldblib.c ldebug.c ldo.c
  ldump.c lfunc.c lgc.c linit.c liolib.c llex.c lmathlib.c lmem.c loadlib.c
  lobject.c lopcodes.c loslib.c lparser.c lstate.c lstring.c lstrlib.c ltable.c
  ltablib.c ltm.c lundump.c lutf8lib.c lvm.c lzio.c)

add_library(lua SHARED ${LUA_CORE})
add_library(lua::lua ALIAS lua)

# Position independent for all (esp. Android)
set_property(TARGET lua PROPERTY POSITION_INDEPENDENT_CODE ON)

target_include_directories(lua PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Platform toggles
if (WIN32)
  target_compile_definitions(lua PRIVATE LUA_BUILD_AS_DLL)
  target_link_libraries(lua PRIVATE ws2_32)
elseif (ANDROID)
  target_compile_definitions(lua PRIVATE LUA_USE_POSIX LUA_USE_DLOPEN)
  target_link_libraries(lua PRIVATE dl log)
else()
  target_compile_definitions(lua PRIVATE LUA_USE_POSIX LUA_USE_DLOPEN)
  target_link_libraries(lua PRIVATE dl m)
endif()

# Nice consistent output name
set_target_properties(lua PROPERTIES OUTPUT_NAME "lua54")