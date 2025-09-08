# Collect all game source files
file(GLOB_RECURSE GAME_SRC CONFIGURE_DEPENDS src/*.cpp src/*.h)

# Build sandbox as an executable
add_executable(Game ${GAME_SRC})

# Link against the engine
target_link_libraries(Game PRIVATE PAINEngine)

# Include engine headers
target_include_directories(Game PRIVATE ${CMAKE_SOURCE_DIR}/PAINEngine/src)
