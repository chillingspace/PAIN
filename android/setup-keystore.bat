@echo off
REM ================================================================
REM Automated Keystore Setup for PAIN Game - Android Release
REM ================================================================
REM This script creates a keystore for signing Android release builds
REM and updates key.properties automatically
REM ================================================================

setlocal enabledelayedexpansion

echo.
echo ============================================
echo   PAIN Game - Android Keystore Setup
echo ============================================
echo.

REM Define keystore location (inside android folder, NOT app folder)
set KEYSTORE_DIR=%~dp0keystore
set KEYSTORE_FILE=%KEYSTORE_DIR%\pain-release.keystore
set KEY_PROPS=%~dp0key.properties

REM Create keystore directory if it doesn't exist
if not exist "%KEYSTORE_DIR%" (
    echo [INFO] Creating keystore directory...
    mkdir "%KEYSTORE_DIR%"
)

REM Check if keystore already exists
if exist "%KEYSTORE_FILE%" (
    echo [INFO] Keystore already exists at: %KEYSTORE_FILE%
    echo.
    choice /M "Do you want to create a new keystore (this will overwrite the old one)"
    if errorlevel 2 goto :UseExisting
    if errorlevel 1 goto :CreateNew
) else (
    goto :CreateNew
)

:CreateNew
echo.
echo ============================================
echo   Creating New Keystore
echo ============================================
echo.

REM Gather information
echo Please provide the following information for your keystore:
echo (Press Enter to use default values shown in brackets)
echo.

set /p KEY_PASSWORD="Key Password [pain2024secure]: "
if "%KEY_PASSWORD%"=="" set KEY_PASSWORD=pain2024secure

set /p STORE_PASSWORD="Store Password [pain2024secure]: "
if "%STORE_PASSWORD%"=="" set STORE_PASSWORD=pain2024secure

set /p KEY_ALIAS="Key Alias [pain_release]: "
if "%KEY_ALIAS%"=="" set KEY_ALIAS=pain_release

set /p VALIDITY="Validity in years [30]: "
if "%VALIDITY%"=="" set VALIDITY=30

REM Calculate validity in days
set /a VALIDITY_DAYS=%VALIDITY%*365

echo.
echo [INFO] Generating keystore with the following details:
echo   - Location: %KEYSTORE_FILE%
echo   - Alias: %KEY_ALIAS%
echo   - Validity: %VALIDITY% years (%VALIDITY_DAYS% days)
echo.

REM ============================================
REM Find keytool (from Android Studio JDK)
REM ============================================
set KEYTOOL=keytool

REM Try to find Android Studio's JDK
echo [INFO] Searching for Android Studio JDK...

REM Common Android Studio JDK locations
set "POSSIBLE_JDKS[0]=%LOCALAPPDATA%\Android\Sdk\jbr\bin\keytool.exe"
set "POSSIBLE_JDKS[1]=%LOCALAPPDATA%\Android\Sdk\jdk\bin\keytool.exe"
set "POSSIBLE_JDKS[2]=%ProgramFiles%\Android\Android Studio\jbr\bin\keytool.exe"
set "POSSIBLE_JDKS[3]=%ProgramFiles%\Android\Android Studio\jre\bin\keytool.exe"
set "POSSIBLE_JDKS[4]=%ProgramFiles(x86)%\Android\Android Studio\jbr\bin\keytool.exe"
set "POSSIBLE_JDKS[5]=%ProgramFiles(x86)%\Android\Android Studio\jre\bin\keytool.exe"

REM Check local.properties for sdk.dir
if exist "%~dp0local.properties" (
    for /f "tokens=1,* delims==" %%a in ('findstr "sdk.dir" "%~dp0local.properties"') do (
        set "SDK_DIR=%%b"
        set "SDK_DIR=!SDK_DIR:\=/!"
        if exist "!SDK_DIR!\jbr\bin\keytool.exe" (
            set "KEYTOOL=!SDK_DIR!\jbr\bin\keytool.exe"
            echo [SUCCESS] Found keytool in Android SDK: !KEYTOOL!
            goto :FoundKeytool
        )
    )
)

REM Try each possible location
for /l %%i in (0,1,5) do (
    if exist "!POSSIBLE_JDKS[%%i]!" (
        set "KEYTOOL=!POSSIBLE_JDKS[%%i]!"
        echo [SUCCESS] Found keytool at: !KEYTOOL!
        goto :FoundKeytool
    )
)

REM If still not found, try PATH
where keytool >nul 2>&1
if %errorlevel% equ 0 (
    echo [SUCCESS] Found keytool in system PATH
    goto :FoundKeytool
)

REM Keytool not found anywhere
echo.
echo [ERROR] Could not find keytool!
echo.
echo Please do ONE of the following:
echo   1. Install Android Studio (which includes JDK)
echo   2. Install Java JDK manually from: https://adoptium.net/
echo   3. If you have Android Studio, find the path to keytool.exe
echo      and add its directory to your system PATH
echo.
echo Common locations to check:
echo   - C:\Users\%USERNAME%\AppData\Local\Android\Sdk\jbr\bin\
echo   - C:\Program Files\Android\Android Studio\jbr\bin\
echo.
pause
exit /b 1

:FoundKeytool
echo.

REM Generate keystore using keytool
"%KEYTOOL%" -genkeypair -v ^
    -keystore "%KEYSTORE_FILE%" ^
    -alias "%KEY_ALIAS%" ^
    -keyalg RSA ^
    -keysize 2048 ^
    -validity %VALIDITY_DAYS% ^
    -storepass "%STORE_PASSWORD%" ^
    -keypass "%KEY_PASSWORD%" ^
    -dname "CN=PAIN Game, OU=Game Development, O=Your Company, L=Singapore, ST=Singapore, C=SG"

if errorlevel 1 (
    echo.
    echo [ERROR] Failed to create keystore!
    echo Make sure you have Java/Android SDK installed.
    pause
    exit /b 1
)

echo.
echo [SUCCESS] Keystore created successfully!
echo.

:UpdateConfig
echo ============================================
echo   Updating key.properties
echo ============================================
echo.

REM Create backup of existing key.properties
if exist "%KEY_PROPS%" (
    copy "%KEY_PROPS%" "%KEY_PROPS%.backup" >nul
    echo [INFO] Backed up existing key.properties to key.properties.backup
)

REM Write new key.properties (using relative path from android/ folder)
(
    echo # Android Release Signing Configuration
    echo # Generated automatically by setup-keystore.bat
    echo # DO NOT COMMIT THIS FILE TO VERSION CONTROL!
    echo.
    echo # Keystore file path ^(relative to android/ directory^)
    echo storeFile=keystore/pain-release.keystore
    echo.
    echo # Keystore passwords
    echo storePassword=%STORE_PASSWORD%
    echo keyPassword=%KEY_PASSWORD%
    echo.
    echo # Key alias
    echo keyAlias=%KEY_ALIAS%
) > "%KEY_PROPS%"

echo [SUCCESS] key.properties updated!
echo.
goto :ShowInfo

:UseExisting
echo.
echo [INFO] Using existing keystore: %KEYSTORE_FILE%
echo.
echo If you need to update key.properties, please provide:
echo.

set /p KEY_PASSWORD="Key Password: "
set /p STORE_PASSWORD="Store Password: "
set /p KEY_ALIAS="Key Alias [pain_release]: "
if "%KEY_ALIAS%"=="" set KEY_ALIAS=pain_release

goto :UpdateConfig

:ShowInfo
echo ============================================
echo   Keystore Information
echo ============================================
echo.
echo Keystore Location: %KEYSTORE_FILE%
echo Key Alias: %KEY_ALIAS%
echo Stored at: android/keystore/pain-release.keystore
echo Referenced in key.properties as: keystore/pain-release.keystore
echo.
echo The path is relative to the android/ folder and will work correctly
echo with your build.gradle.kts configuration.
echo.

echo ============================================
echo   IMPORTANT: Security Reminders
echo ============================================
echo.
echo [!] BACKUP YOUR KEYSTORE: Copy keystore/pain-release.keystore to a secure location
echo [!] KEEP PASSWORDS SAFE: Store passwords in a password manager
echo [!] NEVER COMMIT: Make sure keystore/ and key.properties are in .gitignore
echo.
echo Without the keystore, you CANNOT update your published app!
echo.

REM Check if keystore is in .gitignore
findstr /C:"keystore" "%~dp0.gitignore" >nul 2>&1
if errorlevel 1 (
    echo [WARNING] Adding keystore to .gitignore...
    echo. >> "%~dp0.gitignore"
    echo # Android signing keystore - DO NOT COMMIT >> "%~dp0.gitignore"
    echo keystore/ >> "%~dp0.gitignore"
)

findstr /C:"key.properties" "%~dp0.gitignore" >nul 2>&1
if errorlevel 1 (
    echo [WARNING] Adding key.properties to .gitignore...
    echo key.properties >> "%~dp0.gitignore"
)

echo.
echo ============================================
echo   Setup Complete!
echo ============================================
echo.
echo You can now build release APKs using:
echo   1. Android Studio: Build ^> Select Build Variant ^> release
echo   2. Command line: gradlew assembleRelease
echo.
echo The keystore path is correctly configured to work with both
echo the automated script and your build.gradle.kts!
echo.
pause
exit /b 0
