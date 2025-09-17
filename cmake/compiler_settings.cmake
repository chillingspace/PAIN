# This file sets consistent compiler flags for the entire project.

if(MSVC)
    # --- Set C++ Standard ---
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)

    # --- Set MSVC Runtime Library ---
    # This is the critical fix. It forces all targets to use the static
    # runtime library (/MT for Release, /MTd for Debug), matching the
    # pre-compiled libraries like GLFW and GLEW.
    foreach(CONFIG ${CMAKE_CONFIGURATION_TYPES})
        string(TOUPPER ${CONFIG} UPPER_CONFIG)
        set(CMAKE_CXX_FLAGS_${UPPER_CONFIG} "${CMAKE_CXX_FLAGS_${UPPER_CONFIG}} /MT$<$<CONFIG:Debug>:d>" PARENT_SCOPE)
    endforeach()

    # --- Set Global Compiler Options ---
    # /W4 (high warning level), /EHsc (standard exception handling)
    add_compile_options(/W4 /EHsc)
else()
    # Settings for other compilers like GCC/Clang can go here
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()