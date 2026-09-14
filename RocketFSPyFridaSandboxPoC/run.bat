@echo off
setlocal EnableDelayedExpansion

REM RocketFS Frida Sandbox - Windows Launcher
REM Usage: run.bat [host:port] [--debug]
REM   run.bat 192.168.0.110:8765
REM   run.bat 192.168.0.110:8765 --debug
REM   run.bat --debug  (uses default localhost:8765)

REM --- Parse args ---
set "SERVER=ws://localhost:8765"
set "DEBUG=0"

:parse_args
if "%~1"=="" goto start_app

if /I "%~1"=="--debug" (
    set "DEBUG=1"
    shift
    goto parse_args
)

REM If it doesn't start with --, treat first non-flag arg as the server address
if "%~1"=="--server" (
    set "SERVER=ws://%~2"
    shift
    shift
    goto parse_args
)

set "SERVER=ws://%~1"
shift
goto parse_args

:start_app

echo ====================================================
echo         RocketFS Frida Sandbox
echo ====================================================
echo Server: %SERVER%
if "%DEBUG%"=="1" (
    echo Debug mode enabled
)
echo ====================================================
echo.
echo Press Ctrl+C to stop.
echo.

REM Determine Python paths
set VENV_PYTHON=%~dp0venv\Scripts\python.exe

if not exist "%VENV_PYTHON%" (
    echo [1/3] Creating virtual environment...
    python -m venv venv
    if errorlevel 1 (
        echo ERROR: Failed to create venv. Install Python 3.11+ and try again.
        pause
        exit /b 1
    )
)

echo [2/3] Installing dependencies...
call "%VENV_PYTHON%" -m pip install --upgrade pip -q
call "%VENV_PYTHON%" -m pip install -r requirements.txt -q
if errorlevel 1 (
    echo ERROR: Failed to install dependencies.
    pause
    exit /b 1
)

echo [3/3] Starting launcher...
echo.

if "%DEBUG%"=="1" (
    "%VENV_PYTHON%" run.py --server "!SERVER!" --debug
) else (
    "%VENV_PYTHON%" run.py --server "!SERVER!"
)

echo.
echo Done.
pause
