cmake_minimum_required(VERSION 3.22)
project(ScenePreloadScannerTest LANGUAGES CXX)

add_executable(ScenePreloadScannerTest
  main.cpp
  ${CMAKE_SOURCE_DIR}/PAINEngine/src/PAINEngine/CoreSystems/Scene/ScriptModelPreloadScanner.cpp
)

target_include_directories(ScenePreloadScannerTest PRIVATE
  ${CMAKE_SOURCE_DIR}/PAINEngine/src/PAINEngine
)

set_target_properties(ScenePreloadScannerTest PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests"
  RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/tests"
  RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/tests"
)
