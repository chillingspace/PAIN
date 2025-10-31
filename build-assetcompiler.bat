@echo off

REM Build only AssetCompilerTool.exe in PAIN/build/Tools/AssetCompiler

cd /d "%~dp0build\Tools\AssetCompiler"

REM Try to use MSBuild from PATH (works in CI)
set MSBUILDEXE=MSBuild.exe
where %MSBUILDEXE% >nul 2>nul
if errorlevel 1 (
    REM Not found, fall back to hardcoded local path (customize as needed)
    set "MSBUILDEXE=C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"
)

if exist AssetCompilerTool.vcxproj (
    "%MSBUILDEXE%" AssetCompilerTool.vcxproj /t:Build /m
) else (
    echo [ERROR] Missing AssetCompilerTool.vcxproj in build\Tools\AssetCompiler!
    exit /b 1
)

REM Check for output exe and report
set OUTPUTEXE=AssetCompilerTool.exe
echo [SUCCESS] %OUTPUTEXE% updated/built!
