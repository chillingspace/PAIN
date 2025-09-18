# Macro to import all dependencies
macro(importVcPkgDependencies)
    # ======================= VCPKG Configuration =========================
    message(STATUS "Setting up VcPkg dependencies...")
    #set(VCPKG_CURRENT_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

    #set(VCPKG_ROOT "C:/vcpkg/vcpkg")
    set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "Vcpkg toolchain file")

    # If vcpkg is present, force its toolchain
    if(EXISTS "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
        set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
            CACHE STRING "Vcpkg toolchain file" FORCE)
        message(STATUS "Using vcpkg toolchain: ${CMAKE_TOOLCHAIN_FILE}")
    else()
        message(WARNING "vcpkg not found in ${VCPKG_ROOT}. Did you clone with --recurse-submodules?")
    endif()

    # ==========================================
    # Auto-bootstrap vcpkg
    # ==========================================
    if(EXISTS "${VCPKG_ROOT}" AND NOT EXISTS "${VCPKG_ROOT}/vcpkg")
        message(STATUS "Bootstrapping vcpkg...")
        if(WIN32)
            execute_process(COMMAND "${VCPKG_ROOT}/bootstrap-vcpkg.bat" "-disableMetrics")
        else()
            execute_process(COMMAND "${VCPKG_ROOT}/bootstrap-vcpkg.sh" "-disableMetrics")
        endif()
    endif()

    message(STATUS "All dependencies have been imported successfully.")
endmacro()