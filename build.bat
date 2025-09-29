@echo off
REM ===============================================
REM Windows Build Script for PAIN Engine
REM ===============================================

REM Clean previous build artifacts if they exist
echo [1/3] Cleaning previous build...
if exist build (
    rmdir /s /q build
    echo     Previous build directory removed.
) else (
    echo     No previous build directory found.
)

REM Create build directory and navigate into it
echo [2/3] Creating and configuring project with CMake...
mkdir build
cd build

REM Configure with CMake
cmake .. -G "Visual Studio 17 2022" -A x64
if %errorlevel% neq 0 (
    echo ERROR: CMake configuration failed.
    cd ..
    pause
    exit /b 1
)

REM Build the project
echo [3/3] Building project (Debug configuration)...
cmake --build . --config Debug
if %errorlevel% neq 0 (
    echo ERROR: Build failed.
    cd ..
    pause
    exit /b 1
)

echo.
echo ===============================================
echo Build completed successfully!
echo You can now open PAINEngine.sln in the 'build' folder.
echo ===============================================
echo.

cd ..
pause