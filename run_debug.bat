@echo off
echo ====================================
echo  MySubstitute - Debug Mode (Console Logging)
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

echo Starting MySubstitute in DEBUG MODE (%BUILD_TYPE%)...
echo.
echo *** Console logging enabled ***
echo *** A console window will open showing all debug messages ***
echo.

REM Run with --debug flag to enable console output
echo Starting application with console logging...
start "MySubstitute Debug Console" "%EXECUTABLE%" --debug

echo.
echo ----------------------------------------
echo Debug console window opened!
echo ----------------------------------------
echo.
echo The application is now running with console logging enabled.
echo All std::cout and std::cerr messages will appear in the debug console.
echo.
echo TIP: The application also runs in the system tray.
echo      Right-click the tray icon to access features.
echo.
echo Press any key to close this launcher (application will continue running)...
pause >nul

