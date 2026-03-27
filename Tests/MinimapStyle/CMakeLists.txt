cmake_minimum_required(VERSION 3.22)
project(MinimapStyleTest LANGUAGES CXX)

add_executable(MinimapStyleTest
  main.cpp
  ${CMAKE_SOURCE_DIR}/PAINEngine/src/PAINEngine/Systems/Render/MinimapStyle.cpp
)

target_include_directories(MinimapStyleTest PRIVATE
  ${CMAKE_SOURCE_DIR}/PAINEngine/src/PAINEngine
)

target_link_libraries(MinimapStyleTest PRIVATE glm)

set_target_properties(MinimapStyleTest PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests"
  RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/tests"
  RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/tests"
)
