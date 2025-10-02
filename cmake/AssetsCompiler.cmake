# Define asset root directories
set(PAIN_ASSETS_ROOT "${CMAKE_SOURCE_DIR}/assets")

if (NOT ANDROID)
    # Windows Asset Compilation - Tool runs on host to compile assets for Windows
    add_custom_target(CompileAllAssets
        COMMAND ${CMAKE_COMMAND} -E echo "=== PAINEngine Dynamic Asset Compilation ==="
        COMMAND ${CMAKE_COMMAND} -E make_directory "${PAIN_ASSETS_ROOT}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_SOURCE_DIR}/bin/$<CONFIG>/assets"
        
        COMMAND $<TARGET_FILE:AssetCompilerTool> windows 
                "${PAIN_ASSETS_ROOT}"
                "${CMAKE_SOURCE_DIR}/bin/$<CONFIG>/assets"
                --auto-discover
                
        DEPENDS AssetCompilerTool
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Compiling assets for Windows using host-built tool"
        VERBATIM
    )
    
else()
    # Android Asset Compilation - Tool runs on host to compile assets for Android
    # Note: This should run during development, not during Android app build
    add_custom_target(CompileAllAssetsAndroid
        COMMAND ${CMAKE_COMMAND} -E echo "=== Compiling Assets for Android APK ==="
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_SOURCE_DIR}/android/app/src/main/assets"
        
        # The tool should be built on the host platform and run there
        # This target would normally be run manually or by CI/CD before Android build
        COMMAND echo "Run asset compilation on host platform before Android build"
        
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Assets should be pre-compiled on host before Android build"
        VERBATIM
    )
endif()