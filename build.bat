@echo off
echo ====================================
echo  MySubstitute Virtual Camera Build
echo ====================================
echo.

REM Check if Visual Studio is available
where cl.exe >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo Visual Studio compiler not found in PATH!
    echo Please run this from "Developer Command Prompt for VS 2022"
    echo or run: "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"
    echo.
    pause
    exit /b 1
)

REM Create build directory
echo Creating build directory...
if not exist build (
    mkdir build
)
cd build

REM Run CMake configuration
echo.
echo Configuring with CMake...
cmake .. -G "Visual Studio 17 2022" -A x64

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
cmake --build . --config Debug --verbose

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
echo Executable location: build\bin\Debug\MySubstitute.exe
echo.
echo You can now run: run.bat
echo.
cd ..
pause