@echo off
setlocal

rem === 0) Project Paths ===
set "PROJECT_DIR=%CD%"
set "LIBS_DIR=%PROJECT_DIR%\libs"
set "VCPKG_ROOT=%LIBS_DIR%\vcpkg"

rem === 1) Dynamically find latest Android NDK in the default location ===
echo [INFO] Searching for Android NDK in %LOCALAPPDATA%\Android\Sdk\ndk...
set "DEFAULT_NDK_PATH=%LOCALAPPDATA%\Android\Sdk\ndk"
if not exist "%DEFAULT_NDK_PATH%" (
    echo [ERROR] Default NDK directory not found at "%DEFAULT_NDK_PATH%".
    echo Please install the NDK via Android Studio's SDK Manager.
    pause
    exit /b 1
)

rem Find the latest installed version by sorting directories by name descending
for /f "tokens=*" %%a in ('dir /b /ad /o-n "%DEFAULT_NDK_PATH%"') do (
    set "LATEST_NDK_VERSION=%%a"
    goto :ndk_found
)

echo [ERROR] Could not find any NDK versions inside "%DEFAULT_NDK_PATH%".
pause
exit /b 1

:ndk_found
set "ANDROID_NDK_HOME=%DEFAULT_NDK_PATH%\%LATEST_NDK_VERSION%"
set "ANDROID_NDK=%ANDROID_NDK_HOME%"
echo [INFO] Found and using NDK version: %LATEST_NDK_VERSION%
echo [INFO] ANDROID_NDK_HOME is set to: "%ANDROID_NDK_HOME%"
echo.

rem === 2) Check for Git ===
git --version >nul 2>&1
if errorlevel 1 (
  echo [ERROR] Git is not available on PATH. Please install Git for Windows and retry.
  exit /b 1
)

rem === 3) Clone and Bootstrap vcpkg ===
if not exist "%LIBS_DIR%" (mkdir "%LIBS_DIR%")
if not exist "%VCPKG_ROOT%\.git" (
  echo [INFO] Cloning vcpkg into "%VCPKG_ROOT%"...
  git clone https://github.com/microsoft/vcpkg.git "%VCPKG_ROOT%" || (echo [ERROR] git clone failed.& exit /b 1)
) else (
  echo [INFO] vcpkg already present. Skipping clone.
)
if not exist "%VCPKG_ROOT%\vcpkg.exe" (
  echo [INFO] Bootstrapping vcpkg...
  call "%VCPKG_ROOT%\bootstrap-vcpkg.bat" || (echo [ERROR] bootstrap-vcpkg.bat failed.& exit /b 1)
) else (
  echo [INFO] vcpkg.exe found.
)
echo.

rem === 4) Install all dependencies from vcpkg.json for Android and Windows ===
echo [INFO] Installing dependencies for arm64-android from vcpkg.json...
"%VCPKG_ROOT%\vcpkg.exe" install --triplet arm64-android
if errorlevel 1 (
  echo [ERROR] vcpkg install failed for arm64-android.
  pause
  exit /b 1
)
echo.
echo [INFO] Installing dependencies for x64-windows from vcpkg.json...
"%VCPKG_ROOT%\vcpkg.exe" install --triplet x64-windows
if errorlevel 1 (
  echo [ERROR] vcpkg install failed for x64-windows.
  pause
  exit /b 1
)
echo.

echo [DONE] vcpkg setup complete.
endlocal

cmd /k