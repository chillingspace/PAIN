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

    else()
    # Android builds: Asset compilation happens on the host during build time
    message(STATUS "Skipping AssetCompilerTool for Android build")
endif()
