
# Tools source
file(GLOB_RECURSE TOOLS_SRC CONFIGURE_DEPENDS src/*.cpp src/*.h)

# PAINEngine Build Asset Compiler Tool - ONLY for host platform (not Android)
if(NOT ANDROID)
    # PAINEngine Asset Compiler Tool
    add_executable(AssetCompilerTool ${TOOLS_SRC})

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

    # Unix-specific permissions (only for host builds)
    if(UNIX)
        add_custom_command(TARGET AssetCompilerTool POST_BUILD
            COMMAND chmod +x $<TARGET_FILE:AssetCompilerTool>
        )
    endif()

    # Optional: Link dependencies (when you add them later)
    # target_link_libraries(AssetCompilerTool nlohmann_json::nlohmann_json)

else()
    # Android builds: Asset compilation happens on the host during build time
    # No need to build the AssetCompilerTool for Android
    message(STATUS "Skipping AssetCompilerTool for Android build")
endif()

