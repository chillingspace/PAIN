@echo off
REM ===============================================
REM Clean Script for PAIN Engine
REM ===============================================

echo ===============================================
echo Cleaning PAIN Engine Build Artifacts
echo ===============================================
echo.

set /p CONFIRM="This will delete build/, bin/, bin-int/, and Android caches. Continue? (y/n): "
if /i not "%CONFIRM%"=="y" (
    echo Clean cancelled.
    pause
    exit /b 0
)

echo.
echo Cleaning...

REM Clean CMake build directory
if exist build (
    rmdir /s /q build
    echo [OK] Removed build/
) else (
    echo [SKIP] build/ not found
)

REM Clean output binaries
if exist bin (
    rmdir /s /q bin
    echo [OK] Removed bin/
) else (
    echo [SKIP] bin/ not found
)

REM Clean intermediate files
if exist bin-int (
    rmdir /s /q bin-int
    echo [OK] Removed bin-int/
) else (
    echo [SKIP] bin-int/ not found
)

REM Clean Android cache
if exist android\.idea (
    rmdir /s /q android\.idea
    echo [OK] android\.idea
) else (
    echo [SKIP] android\.idea not found
)

if exist android\.gradle (
    rmdir /s /q android\.gradle
    echo [OK] android\.gradle
) else (
    echo [SKIP] android\.gradle not found
)

if exist android\app\build (
    rmdir /s /q android\app\build
    echo [OK] Removed android\app\build
) else (
    echo [SKIP] android\app\build not found
)

REM Optional: Clean FetchContent cache (uncomment if needed)
REM if exist _deps (
REM     rmdir /s /q _deps
REM     echo [OK] Removed _deps/
REM )

echo.
echo ===============================================
echo Clean complete! Run build.bat to rebuild.
echo ===============================================
pause