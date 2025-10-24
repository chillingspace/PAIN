cmake_minimum_required(VERSION 3.22)
project(lua_vendor LANGUAGES C)

file(GLOB LUA_SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

# On Android, build a static library with appropriate properties
add_library(lua STATIC ${LUA_SRC})
target_include_directories(lua PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

if(ANDROID)
    target_link_libraries(lua PUBLIC android log)
endif()

set_target_properties(lua PROPERTIES OUTPUT_NAME "lua")
