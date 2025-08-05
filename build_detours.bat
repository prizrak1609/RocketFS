@echo off
REM Check if running in Developer Command Prompt by looking for cl.exe in PATH
where cl.exe >nul 2>nul
if errorlevel 1 (
    echo Error: This script must be run from a Visual Studio Developer Command Prompt.
    exit /b 1
)

REM Set variables
set "REPO_URL=https://github.com/microsoft/Detours.git"
set "TAG=main"
set "BUILD_DIR=build/Detours"

REM Clone the repository with the specific tag
if not exist "%BUILD_DIR%" (
    git clone --branch %TAG% --depth 1 %REPO_URL% "%BUILD_DIR%"
) else (
    echo Directory %BUILD_DIR% already exists. Skipping clone.
)

REM Change to build directory
cd /d "%BUILD_DIR%"

REM Build with nmake and environment variables
nmake DETOURS_CONFIG="Release"
