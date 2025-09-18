@echo off
setlocal DisableDelayedExpansion

rem === 0) Use the current working directory (project root) ===
set "PROJECT_DIR=%CD%"
set "LIBS_DIR=%PROJECT_DIR%\libs"
set "VCPKG_ROOT=%LIBS_DIR%\vcpkg"

rem === DYNAMICALLY FIND ANDROID_NDK_HOME ===
echo [INFO] Searching for Android NDK...

rem Check if ANDROID_NDK_HOME is already set as an environment variable
if defined ANDROID_NDK_HOME (
    echo [INFO] Found ANDROID_NDK_HOME in environment variables.
) else (
    echo [INFO] ANDROID_NDK_HOME not set. Searching default location...
    set "DEFAULT_NDK_PATH=%LOCALAPPDATA%\Android\Sdk\ndk"
    if exist "%DEFAULT_NDK_PATH%" (
        rem Find the latest installed version by sorting directories
        for /f "tokens=*" %%a in ('dir /b /ad /o-n "%DEFAULT_NDK_PATH%"') do (
            set "LATEST_NDK_VERSION=%%a"
            goto :ndk_found
        )
    )
    
    echo [ERROR] Could not find NDK automatically.
    echo Please set the ANDROID_NDK_HOME environment variable to your NDK path.
    echo Example: C:\Users\YourUser\AppData\Local\Android\Sdk\ndk\27.0.12077973
    pause
    exit /b 1

    :ndk_found
    set "ANDROID_NDK_HOME=%DEFAULT_NDK_PATH%\%LATEST_NDK_VERSION%"
    echo [INFO] Found latest NDK version at: %LATEST_NDK_VERSION%
)

set "ANDROID_NDK=%ANDROID_NDK_HOME%"
echo ANDROID_NDK_HOME=%ANDROID_NDK_HOME%
echo.

echo [INFO] Project: %PROJECT_DIR%
echo [INFO] libs dir: %LIBS_DIR%
echo [INFO] vcpkg   : %VCPKG_ROOT%
echo.

rem === 1) Check that Git is available ===
git --version >nul 2>&1
if errorlevel 1 (
  echo [ERROR] Git is not available on PATH. Please install Git for Windows and retry.
  exit /b 1
)

rem === 2) Ensure libs directory exists ===
if not exist "%LIBS_DIR%" (
  mkdir "%LIBS_DIR%" || (
    echo [ERROR] Failed to create libs directory: %LIBS_DIR%
    exit /b 1
  )
)

rem === 3) Clone vcpkg into libs\vcpkg if missing ===
if not exist "%VCPKG_ROOT%\.git" (
  echo [INFO] Cloning vcpkg into "%VCPKG_ROOT%" ...
  git clone https://github.com/microsoft/vcpkg.git "%VCPKG_ROOT%" || (
    echo [ERROR] git clone failed.
    exit /b 1
  )
) else (
  echo [INFO] vcpkg already present at "%VCPKG_ROOT%". Skipping clone.
)
echo.

rem === 4) Bootstrap vcpkg.exe if needed ===
if not exist "%VCPKG_ROOT%\vcpkg.exe" (
  echo [INFO] Bootstrapping vcpkg...
  call "%VCPKG_ROOT%\bootstrap-vcpkg.bat" || (
    echo [ERROR] bootstrap-vcpkg.bat failed.
    exit /b 1
  )
) else (
  echo [INFO] vcpkg.exe found.
)
echo.

rem === 5) Install Dependencies for Android ===
echo [INFO] Installing dependencies for Android...
"%VCPKG_ROOT%\vcpkg.exe" install --triplet arm64-android --classic
if errorlevel 1 (
  echo [ERROR] vcpkg install failed for arm64-android.
  exit /b 1
)
echo [OK] Dependencies installed for arm64-android.
echo.

echo [DONE] vcpkg setup complete.
echo [HINT] Configure CMake with:
echo   -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake"
echo   -DVCPKG_CHAINLOAD_TOOLCHAIN_FILE="!ANDROID_NDK_HOME!\build\cmake\android.toolchain.cmake"
echo   -DVCPKG_TARGET_TRIPLET=arm64-android

endlocal