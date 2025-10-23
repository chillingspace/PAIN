# Define asset root directories
set(PAIN_ASSETS_ROOT "${CMAKE_SOURCE_DIR}/assets")

# --- Safety: ensure bin/<cfg> are directories, not stray files ---
if (NOT ANDROID)
  foreach(cfg Debug Release RelWithDebInfo MinSizeRel)
    set(_bin_cfg "${CMAKE_SOURCE_DIR}/bin/${cfg}")
    if (EXISTS "${_bin_cfg}" AND NOT IS_DIRECTORY "${_bin_cfg}")
      message(WARNING "Found a FILE at ${_bin_cfg}; removing so CMake can create the directory.")
      file(REMOVE "${_bin_cfg}")
    endif()
  endforeach()
endif()

if (NOT ANDROID)
    set(_OUT_ASSETS_DIR "${CMAKE_SOURCE_DIR}/bin/$<CONFIG>/Assets")

    # Windows Asset Compilation - Tool runs on host to compile assets for Windows
    add_custom_target(CompileAllAssets
        COMMAND ${CMAKE_COMMAND} -E echo "=== PAINEngine Dynamic Asset Compilation ==="
        COMMAND ${CMAKE_COMMAND} -E make_directory "${PAIN_ASSETS_ROOT}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${_OUT_ASSETS_DIR}"
        
        COMMAND $<TARGET_FILE:AssetCompilerTool> windows 
                "${PAIN_ASSETS_ROOT}"
                "${_OUT_ASSETS_DIR}"
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