set(ASSETS_INPUT_DIR "${CMAKE_SOURCE_DIR}/assets")

if(WIN32)
    set(ASSET_COMPILER_EXE "${CMAKE_BINARY_DIR}/Tools/AssetCompilerTool.exe")
    set(GAME_ASSET_OUT "${CMAKE_SOURCE_DIR}/bin/$<CONFIG>/assets")

    add_custom_target(CompileAllAssets ALL
        COMMAND ${CMAKE_COMMAND} -E make_directory "${GAME_ASSET_OUT}"
        COMMAND "${ASSET_COMPILER_EXE}"
            --input "${ASSETS_INPUT_DIR}"
            --output "${GAME_ASSET_OUT}"
            --target "windows"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        COMMENT "Running asset compiler before running the game"
        VERBATIM
        USES_TERMINAL
    )

    add_dependencies(CompileAllAssets AssetCompilerTool)

elseif(ANDROID)
    set(ASSET_COMPILER_EXE "${CMAKE_SOURCE_DIR}/build/Tools/AssetCompilerTool.exe")
    set(ASSETS_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/android/app/src/main/assets")

    add_custom_target(CompileAllAssets ALL
        COMMAND ${CMAKE_COMMAND} -E make_directory "${ASSETS_OUTPUT_DIR}"
        COMMAND "${ASSET_COMPILER_EXE}"
            --input "${ASSETS_INPUT_DIR}"
            --output "${ASSETS_OUTPUT_DIR}"
            --target "android"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        COMMENT "Running asset compiler before packaging APK"
        VERBATIM
        USES_TERMINAL
    )

endif()
