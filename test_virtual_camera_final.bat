@echo off
echo ========================================
echo MySubstitute Virtual Camera Final Test
echo ========================================

echo.
echo 1. Checking if DirectShow DLL exists...
if exist "build\bin\Release\MySubstituteVirtualCamera.dll" (
    echo ✅ DLL exists: build\bin\Release\MySubstituteVirtualCamera.dll
    dir "build\bin\Release\MySubstituteVirtualCamera.dll" | findstr MySubstitute
) else (
    echo ❌ DLL not found!
    exit /b 1
)

echo.
echo 2. Checking CLSID registration...
reg query "HKEY_CLASSES_ROOT\CLSID\{B3F3A1C4-8F9E-4A2D-9B5C-7E6F8D4C9A3B}" >nul 2>&1
if %errorlevel%==0 (
    echo ✅ CLSID registered
    reg query "HKEY_CLASSES_ROOT\CLSID\{B3F3A1C4-8F9E-4A2D-9B5C-7E6F8D4C9A3B}" | findstr "MySubstitute"
) else (
    echo ❌ CLSID not registered!
)

echo.
echo 3. Checking DirectShow category registration...
reg query "HKEY_CLASSES_ROOT\CLSID\{860BB310-5D01-11d0-BD3B-00A0C911CE86}\Instance\{B3F3A1C4-8F9E-4A2D-9B5C-7E6F8D4C9A3B}" >nul 2>&1
if %errorlevel%==0 (
    echo ✅ DirectShow category registered
    reg query "HKEY_CLASSES_ROOT\CLSID\{860BB310-5D01-11d0-BD3B-00A0C911CE86}\Instance\{B3F3A1C4-8F9E-4A2D-9B5C-7E6F8D4C9A3B}" | findstr "FriendlyName\|MySubstitute"
) else (
    echo ❌ DirectShow category not registered!
)

echo.
echo 4. Testing DLL registration status...
regsvr32 /s "build\bin\Release\MySubstituteVirtualCamera.dll"
if %errorlevel%==0 (
    echo ✅ DLL registration successful
) else (
    echo ❌ DLL registration failed with exit code %errorlevel%
)

echo.
echo 5. Checking registration log...
if exist "C:\temp\directshow_registration.log" (
    echo ✅ Registration log exists:
    type "C:\temp\directshow_registration.log"
) else (
    echo ❌ No registration log found
)

echo.
echo ========================================
echo Test completed!
echo ========================================
echo.
echo Next step: Check https://webcamtests.com for "MySubstitute Virtual Camera"
pause