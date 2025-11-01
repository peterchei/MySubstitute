@echo off
echo ====================================
echo  Running MySubstitute Virtual Camera
echo ====================================
echo.

REM Check if executable exists
if not exist build\bin\Debug\MySubstitute.exe (
    echo MySubstitute.exe not found!
    echo Please build the project first using: build.bat
    echo.
    pause
    exit /b 1
)

REM Run the application
echo Starting MySubstitute...
echo.
cd build\bin\Debug
MySubstitute.exe

echo.
echo Application finished.
pause