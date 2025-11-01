@echo off
echo ====================================
echo  Running MySubstitute Virtual Camera
echo ====================================
echo.

REM Check if executable exists (debug version first, then release)
if exist build\bin\Debug\MySubstitute_d.exe (
    set EXECUTABLE=build\bin\Debug\MySubstitute_d.exe
    set BUILD_TYPE=Debug
) else if exist build\bin\Release\MySubstitute.exe (
    set EXECUTABLE=build\bin\Release\MySubstitute.exe
    set BUILD_TYPE=Release
) else (
    echo MySubstitute executable not found!
    echo Please build the project first using: build.bat
    echo.
    pause
    exit /b 1
)

REM Run the application
echo Starting MySubstitute (%BUILD_TYPE%)...
echo.
%EXECUTABLE%

echo.
echo Application finished.
pause