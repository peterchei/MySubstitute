@echo off
echo ========================================
echo Testing MySubstitute Virtual Camera
echo ========================================
echo.

cd /d "c:\Users\peter\git\MySubstitute\build\bin\Release"

echo Checking if running as administrator...
net session >nul 2>&1
if %errorLevel% == 0 (
    echo ✅ Running as Administrator
    echo.
    echo Testing virtual camera registration...
    MySubstitute.exe --test-virtual-camera
    echo.
    echo Registration test completed!
    echo.
    echo Opening Camera app to test visibility...
    start ms-windows-store://pdp/?ProductId=9WZDNCRFJBBG
) else (
    echo ❌ Not running as Administrator
    echo Please run this batch file as Administrator
    echo Right-click → "Run as administrator"
)

echo.
echo Press any key to exit...
pause >nul