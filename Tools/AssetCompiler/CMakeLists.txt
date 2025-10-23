# PAINEngine Build Asset Compiler Tool - ONLY for host platform (not Android)
if(NOT ANDROID)
    # Find main.cpp for the executable
    set(TOOL_MAIN_SRC "")
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")
        set(TOOL_MAIN_SRC main.cpp)
    elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")
        set(TOOL_MAIN_SRC src/main.cpp)
    else()
        message(FATAL_ERROR "Could not find main.cpp for AssetCompilerTool")
    endif()

    # PAINEngine Asset Compiler Tool Executable
    add_executable(AssetCompilerTool ${TOOL_MAIN_SRC})

    # Link to the Asset Pipeline library (not duplicate sources)
    target_link_libraries(AssetCompilerTool
        PRIVATE PAINAssetPipeline  # This contains all your logic
    )

    # Set C++ standard
    set_property(TARGET AssetCompilerTool PROPERTY CXX_STANDARD 17)
    set_property(TARGET AssetCompilerTool PROPERTY CXX_STANDARD_REQUIRED ON)

    # Set output directory - build tools go to build/tools/
    set_target_properties(AssetCompilerTool PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tools"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/tools"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/tools"
        RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/tools"
        RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_BINARY_DIR}/tools"
    )

    # Windows-specific console application settings
    if(WIN32)
        set_target_properties(AssetCompilerTool PROPERTIES
            WIN32_EXECUTABLE FALSE  # Console application
        )
    endif()

    # Unix-specific permissions
    if(UNIX)
        add_custom_command(TARGET AssetCompilerTool POST_BUILD
            COMMAND chmod +x $<TARGET_FILE:AssetCompilerTool>
        )
    endif()

    # ================================================
    # Post-build: Copy Lua DLL so the tool runs properly
    # ================================================
    add_custom_command(TARGET AssetCompilerTool POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "${CMAKE_BINARY_DIR}/vendor_lua/$<CONFIG>/lua54$<$<CONFIG:Debug>:d>.dll"
      "$<TARGET_FILE_DIR:AssetCompilerTool>/"
  )

  # (optional) also drop a copy into bin/<config> for manual runs
  add_custom_command(TARGET AssetCompilerTool POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_SOURCE_DIR}/bin/$<CONFIG>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "${CMAKE_BINARY_DIR}/vendor_lua/$<CONFIG>/lua54$<$<CONFIG:Debug>:d>.dll"
      "${CMAKE_SOURCE_DIR}/bin/$<CONFIG>/"
  )

else()
    # Android builds: Asset compilation happens on the host during build time
    message(STATUS "Skipping AssetCompilerTool for Android build")
endif()
