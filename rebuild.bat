@echo off
echo ================================================
echo  Rebuilding MySubstitute with 5 AI Styles
echo ================================================
echo.
echo Please close MySubstitute.exe if it's running...
echo.
pause

:retry
echo.
echo Attempting to build...
cmake --build build --config Release

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ================================================
    echo  Build successful!
    echo ================================================
    echo.
    echo You can now run: .\build\bin\Release\MySubstitute.exe
    echo.
    echo The menu now has 5 AI styles:
    echo   - AI Style: Candy
    echo   - AI Style: Mosaic
    echo   - AI Style: Starry Night
    echo   - AI Style: La Muse
    echo   - AI Style: Feathers
    echo.
) else (
    echo.
    echo Build failed - MySubstitute.exe may still be running
    echo Please close it and press any key to retry...
    pause >nul
    goto retry
)

pause
