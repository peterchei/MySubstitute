@echo off
echo Testing DirectShow Virtual Camera DLL
echo =====================================

cd /d "c:\Users\peter\git\MySubstitute\build\bin\Release"

echo.
echo 1. Checking DLL file...
if exist MySubstituteVirtualCamera.dll (
    echo ✅ DLL found: MySubstituteVirtualCamera.dll
    dir MySubstituteVirtualCamera.dll
) else (
    echo ❌ DLL not found!
    goto :end
)

echo.
echo 2. Unregistering existing registration...
regsvr32 /u /s MySubstituteVirtualCamera.dll

echo.
echo 3. Registering DirectShow DLL...
regsvr32 /s MySubstituteVirtualCamera.dll
if %errorlevel% == 0 (
    echo ✅ Registration successful
) else (
    echo ❌ Registration failed
    goto :end
)

echo.
echo 4. Checking registry entries...
reg query "HKEY_CLASSES_ROOT\CLSID\{B3F3A1C4-8F9E-4A2D-9B5C-7E6F8D4C9A3B}" >nul 2>&1
if %errorlevel% == 0 (
    echo ✅ CLSID registered
) else (
    echo ❌ CLSID not found
)

reg query "HKEY_CLASSES_ROOT\CLSID\{860BB310-5D01-11d0-BD3B-00A0C911CE86}\Instance" | findstr MySubstitute >nul 2>&1
if %errorlevel% == 0 (
    echo ✅ DirectShow category registered
) else (
    echo ❌ DirectShow category not found
)

echo.
echo 5. Testing webcam detection...
echo Please test in webcam test website now.
echo The virtual camera should appear as "MySubstitute Virtual Camera"
echo.

:end
pause