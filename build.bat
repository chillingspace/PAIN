@echo off
REM =========================
REM PAINEngine Build Script
REM =========================

REM Set build configuration: Debug or Release
set BUILD_TYPE=Debug

REM Set CMake generator (adjust if needed)
set CMAKE_GENERATOR="Visual Studio 17 2022"

REM Optional: Clean previous build folder
if exist build (
    echo Cleaning previous build...
    rmdir /s /q build
)

REM Create build folder
mkdir build
cd build

REM Configure CMake project
cmake -G %CMAKE_GENERATOR% -A x64 -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ..

REM Build the project
cmake --build . --config %BUILD_TYPE%

REM Done
echo =========================
echo Build finished!
echo =========================
pause
