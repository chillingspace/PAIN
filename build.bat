@echo off
REM ===============================================
REM Windows Build Script for PAIN Engine
REM ===============================================

REM Parse build configuration argument (default to Debug)
set BUILD_CONFIG=Debug
if "%1"=="release" set BUILD_CONFIG=Release
if "%1"=="Release" set BUILD_CONFIG=Release
if "%1"=="debug" set BUILD_CONFIG=Debug
if "%1"=="Debug" set BUILD_CONFIG=Debug

echo Building in %BUILD_CONFIG% mode...
echo.

REM Configure project if build directory does not exist
if not exist build (
    echo [1/2] Creating build directory and configuring project...
    mkdir build
    cd build
    cmake .. -G "Visual Studio 17 2022" -A x64
    if %errorlevel% neq 0 (
        echo ERROR: CMake configuration failed.
        cd ..
        pause
        exit /b 1
    )
    cd ..
) else (
    echo [1/2] Build directory exists, skipping configure...
)

cd build

REM Determine parallel job count
if defined CMAKE_BUILD_PARALLEL_LEVEL (
    set PARALLEL_JOBS=%CMAKE_BUILD_PARALLEL_LEVEL%
) else (
    set PARALLEL_JOBS=%NUMBER_OF_PROCESSORS%
)

REM Build the project with multithreading
echo [2/2] Building project (%BUILD_CONFIG% configuration) with %PARALLEL_JOBS% parallel jobs...
cmake --build . --config %BUILD_CONFIG% -j %PARALLEL_JOBS%
if %errorlevel% neq 0 (
    echo ERROR: Build failed.
    cd ..
    pause
    exit /b 1
)

echo.
echo ===============================================
echo %BUILD_CONFIG% build completed successfully!
echo You can now open PAINEngine.sln in the 'build' folder.
echo ===============================================
echo.

cd ..
pause