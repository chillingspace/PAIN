@echo off
REM Build only AssetCompilerTool.exe in PAIN/build/Tools/AssetCompiler

cd /d "%~dp0build\Tools\AssetCompiler"

REM Update the MSBUILD path below if using a different VS edition (e.g. Professional, Enterprise)
set "MSBUILD_EXE=C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"

if not exist "%MSBUILD_EXE%" (
    echo [ERROR] Could not find MSBuild.exe at "%MSBUILD_EXE%"
    echo Please update this script with your correct Visual Studio path.
    pause
    exit /b 1
)

if exist AssetCompilerTool.vcxproj (
    "%MSBUILD_EXE%" AssetCompilerTool.vcxproj /t:Build /m
) else (
    echo [ERROR] Missing AssetCompilerTool.vcxproj in build\Tools\AssetCompiler!
    pause
    exit /b 1
)

REM Check for output exe and report
set OUTPUTEXE=AssetCompilerTool.exe
echo [SUCCESS] %OUTPUTEXE% updated/built!
pause


