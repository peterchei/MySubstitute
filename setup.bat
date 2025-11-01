@echo off
echo ====================================
echo  MySubstitute Development Setup
echo ====================================
echo.

echo Checking requirements...
echo.

REM Check for Visual Studio
echo 1. Checking Visual Studio...
where cl.exe >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo    ✓ Visual Studio compiler found
    for /f "tokens=*" %%i in ('cl.exe 2^>^&1 ^| findstr "Version"') do echo    %%i
) else (
    echo    ✗ Visual Studio compiler not found
    echo    Please install Visual Studio 2022 Community Edition with C++ workload
    echo    Download from: https://visualstudio.microsoft.com/vs/community/
)

REM Check for CMake
echo.
echo 2. Checking CMake...
where cmake.exe >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo    ✓ CMake found
    for /f "tokens=3" %%i in ('cmake --version ^| findstr "version"') do echo    Version: %%i
) else (
    echo    ✗ CMake not found
    echo    CMake is usually included with Visual Studio C++ workload
    echo    Or download separately from: https://cmake.org/download/
)

REM Check for Git
echo.
echo 3. Checking Git...
where git.exe >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo    ✓ Git found
    for /f "tokens=3" %%i in ('git --version') do echo    Version: %%i
) else (
    echo    ✗ Git not found (optional)
    echo    Download from: https://git-scm.com/download/win
)

REM Check for OpenCV (optional)
echo.
echo 4. Checking OpenCV (optional)...
if defined OpenCV_DIR (
    if exist "%OpenCV_DIR%" (
        echo    ✓ OpenCV directory found: %OpenCV_DIR%
    ) else (
        echo    ✗ OpenCV_DIR points to non-existent directory
    )
) else (
    echo    ⚠ OpenCV_DIR not set (optional - project will build without it)
    echo    To enable OpenCV features:
    echo    1. Download OpenCV from: https://opencv.org/releases/
    echo    2. Extract to C:\opencv
    echo    3. Set environment variable: OpenCV_DIR=C:\opencv\build
    echo    4. Add to PATH: C:\opencv\build\x64\vc16\bin
)

echo.
echo ====================================
echo  Setup Status
echo ====================================

REM Check if ready to build
where cl.exe >nul 2>&1
if %ERRORLEVEL% NEQ 0 goto :not_ready

where cmake.exe >nul 2>&1
if %ERRORLEVEL% NEQ 0 goto :not_ready

echo ✓ Ready to build! You can run: build.bat
goto :end

:not_ready
echo ✗ Missing required components. Please install:
echo   - Visual Studio 2022 with C++ workload
echo   - CMake (included with Visual Studio)

:end
echo.
echo For detailed setup instructions, see: docs\development_setup.md
echo.
pause