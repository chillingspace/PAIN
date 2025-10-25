# Define input sources as before
set(ASSETS_INPUT_DIR "${CMAKE_SOURCE_DIR}/assets")
file(GLOB_RECURSE ALL_ASSET_INPUTS "${ASSETS_INPUT_DIR}/*")

if(WIN32)
    set(ASSET_COMPILER_EXE "${CMAKE_BINARY_DIR}/Tools/AssetCompilerTool.exe")

    add_custom_command(
        OUTPUT "${CMAKE_SOURCE_DIR}/bin/$<CONFIG>/assets/.assets_compiled_stamp"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_SOURCE_DIR}/bin/$<CONFIG>/assets"
        COMMAND "${ASSET_COMPILER_EXE}"
            --input "${ASSETS_INPUT_DIR}"
            --output "${CMAKE_SOURCE_DIR}/bin/$<CONFIG>/assets"
            --target "windows"
        COMMAND ${CMAKE_COMMAND} -E touch "${CMAKE_SOURCE_DIR}/bin/$<CONFIG>/assets/.assets_compiled_stamp"
        DEPENDS ${ASSET_COMPILER_EXE} ${ALL_ASSET_INPUTS}
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        COMMENT "Building assets for ${CMAKE_SYSTEM_NAME}"
        VERBATIM
    )

    add_custom_target(CompileAllAssets
        DEPENDS "${CMAKE_SOURCE_DIR}/bin/$<CONFIG>/assets/.assets_compiled_stamp"
    )
elseif(ANDROID)
    set(ASSET_COMPILER_EXE "${CMAKE_SOURCE_DIR}/build/Tools/AssetCompilerTool.exe")
    set(ASSETS_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/android/app/src/main/assets") # Android assets for APK

    add_custom_command(
        OUTPUT "${ASSETS_OUTPUT_DIR}/.assets_compiled_stamp"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${ASSETS_OUTPUT_DIR}"
        COMMAND "${ASSET_COMPILER_EXE}"
            --input "${ASSETS_INPUT_DIR}"
            --output "${ASSETS_OUTPUT_DIR}"
            --target "android"
        COMMAND ${CMAKE_COMMAND} -E touch "${ASSETS_OUTPUT_DIR}/.assets_compiled_stamp"
        DEPENDS ${ASSET_COMPILER_EXE} ${ALL_ASSET_INPUTS}
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        COMMENT "Building assets for ${CMAKE_SYSTEM_NAME}"
        VERBATIM
    )

    add_custom_target(CompileAllAssets
        DEPENDS "${ASSETS_OUTPUT_DIR}/.assets_compiled_stamp"
    )
endif()
