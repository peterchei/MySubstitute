@echo off
echo ====================================
echo  MySubstitute Virtual Camera Build
echo ====================================
echo.

REM Check if Visual Studio is available
where cl.exe >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo Visual Studio compiler not in PATH, attempting to load VS environment...
    if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" (
        call "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" >nul
        echo Visual Studio environment loaded.
    ) else (
        echo ERROR: Visual Studio 2022 not found!
        echo Please install Visual Studio 2022 with Desktop C++ workload.
        echo.
        pause
        exit /b 1
    )
)

REM Create build directory
echo Creating build directory...
if not exist build (
    mkdir build
)

REM Clear CMake cache if it exists (to avoid platform mismatch)
if exist build\CMakeCache.txt (
    echo Clearing CMake cache...
    del /F /Q build\CMakeCache.txt >nul 2>&1
)

REM Run CMake configuration
echo.
echo Configuring with CMake...
echo  - Enabling ONNX Runtime support
cmake -B build -G "Visual Studio 17 2022" -A x64 -DUSE_ONNX=ON

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo CMake configuration failed!
    echo Make sure CMake is installed and available in PATH.
    echo.
    pause
    exit /b 1
)

REM Build the project
echo.
echo Building project...
cmake --build build --config Debug --verbose

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo Build failed! Check the errors above.
    echo.
    pause
    exit /b 1
)

echo.
echo ====================================
echo  Build completed successfully!
echo ====================================
echo.
echo Executable location: build\bin\Debug\MySubstitute_d.exe
echo.
echo You can now run: run.bat or run_debug.bat
echo.
pause