@echo off
REM
REM Voxel Engine Automated Benchmark Launcher (Windows)
REM Compiles the extension (if needed) and runs performance tests
REM

setlocal enabledelayedexpansion

REM Configuration
set SCRIPT_DIR=%~dp0
set PROJECT_DIR=%SCRIPT_DIR%project
set GODOT_BIN=%GODOT_BIN%
set PLATFORM=windows
set TARGET=template_debug
set JOBS=9
set SKIP_BUILD=false
set REBUILD=false

REM Default Godot path if not set
if "%GODOT_BIN%"=="" (
    set GODOT_BIN=C:\Program Files\Godot\Godot.exe
)

echo ========================================
echo   Voxel Engine Benchmark System
echo ========================================
echo.

REM Parse arguments
:parse_args
if "%~1"=="" goto args_done
if "%~1"=="--skip-build" (
    set SKIP_BUILD=true
    shift
    goto parse_args
)
if "%~1"=="--rebuild" (
    set REBUILD=true
    shift
    goto parse_args
)
if "%~1"=="--release" (
    set TARGET=template_release
    shift
    goto parse_args
)
if "%~1"=="--jobs" (
    set JOBS=%~2
    shift
    shift
    goto parse_args
)
if "%~1"=="--help" (
    echo Usage: run_benchmark.bat [OPTIONS]
    echo.
    echo Options:
    echo   --skip-build       Skip compilation, run benchmark only
    echo   --rebuild          Clean and rebuild extension
    echo   --release          Use release build instead of debug
    echo   --jobs N           Parallel compilation jobs (default: 9^)
    echo   --help             Show this help message
    echo.
    echo Environment Variables:
    echo   GODOT_BIN          Path to Godot executable
    echo                      (default: C:\Program Files\Godot\Godot.exe^)
    exit /b 0
)
shift
goto parse_args
:args_done

REM Check if Godot exists
if not exist "%GODOT_BIN%" (
    echo Error: Godot not found at: %GODOT_BIN%
    echo Set GODOT_BIN environment variable or install Godot to default path
    exit /b 1
)

REM Build extension
if "%SKIP_BUILD%"=="false" (
    echo [1/2] Building Extension...
    echo Platform: %PLATFORM% ^| Target: %TARGET% ^| Jobs: %JOBS%
    echo.

    if "%REBUILD%"=="true" (
        echo Cleaning build artifacts...
        scons -c platform=%PLATFORM% target=%TARGET% 2>nul
    )

    scons platform=%PLATFORM% target=%TARGET% -j%JOBS%

    if errorlevel 1 (
        echo Error: Build failed
        exit /b 1
    )
    echo Build complete!
    echo.
) else (
    echo [1/2] Skipping build (--skip-build^)
    echo.
)

REM Run benchmark
echo [2/2] Running Benchmark...
echo Scene: res://benchmark/benchmark.tscn
echo.

REM Create results directory if it doesn't exist
if not exist "%PROJECT_DIR%\.godot\benchmark_results" (
    mkdir "%PROJECT_DIR%\.godot\benchmark_results"
)

REM Launch Godot with benchmark scene
"%GODOT_BIN%" --path "%PROJECT_DIR%" --scene res://benchmark/benchmark.tscn --quit-after 1800

set EXIT_CODE=%ERRORLEVEL%

if %EXIT_CODE%==0 (
    echo.
    echo ========================================
    echo   Benchmark Complete!
    echo ========================================
    echo.
    echo Results saved to:
    echo   %%APPDATA%%\Godot\app_userdata\game\benchmark_results\
    echo.
) else (
    echo Benchmark exited with code: %EXIT_CODE%
)

exit /b %EXIT_CODE%
