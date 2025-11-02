@echo off
echo ========================================
echo Testing SimpleRegistryVirtualCamera
echo ========================================
echo.

cd /d "c:\Users\peter\git\MySubstitute\build\bin\Release"

echo Running virtual camera registration test...
echo Please check the system tray or any dialog boxes that appear.
echo.

MySubstitute.exe

echo.
echo Test completed. Press any key to exit.
pause