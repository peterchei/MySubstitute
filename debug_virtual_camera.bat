@echo off
echo ========================================
echo DirectShow Virtual Camera Debug Test
echo ========================================

echo.
echo 1. Checking CLSID registration...
reg query "HKEY_CLASSES_ROOT\CLSID\{B3F3A1C4-8F9E-4A2D-9B5C-7E6F8D4C9A3B}" >nul 2>&1
if %errorlevel%==0 (
    echo ✅ CLSID registered
    echo Details:
    reg query "HKEY_CLASSES_ROOT\CLSID\{B3F3A1C4-8F9E-4A2D-9B5C-7E6F8D4C9A3B}"
    echo.
    echo InprocServer32:
    reg query "HKEY_CLASSES_ROOT\CLSID\{B3F3A1C4-8F9E-4A2D-9B5C-7E6F8D4C9A3B}\InprocServer32"
) else (
    echo ❌ CLSID not registered!
    exit /b 1
)

echo.
echo 2. Checking DLL file existence...
set "DLL_PATH=C:\Users\peter\git\MySubstitute\build\bin\Release\MySubstituteVirtualCamera.dll"
if exist "%DLL_PATH%" (
    echo ✅ DLL exists: %DLL_PATH%
    dir "%DLL_PATH%" | findstr MySubstitute
) else (
    echo ❌ DLL not found at: %DLL_PATH%
    exit /b 1
)

echo.
echo 3. Testing DLL loading with regsvr32...
regsvr32 /s "%DLL_PATH%"
set "REG_RESULT=%errorlevel%"
echo regsvr32 exit code: %REG_RESULT%

if %REG_RESULT%==0 (
    echo ✅ DLL registration successful
) else (
    echo ❌ DLL registration failed - possible causes:
    echo   - Missing dependencies
    echo   - Invalid DLL exports  
    echo   - COM interface issues
    echo   - Access permissions
)

echo.
echo 4. Checking Windows Event Log for errors...
powershell "Get-WinEvent -LogName Application -MaxEvents 5 | Where-Object {$_.LevelDisplayName -eq 'Error' -and $_.TimeCreated -gt (Get-Date).AddMinutes(-5)} | Format-Table TimeCreated,Id,LevelDisplayName,Message -Wrap"

echo.
echo 5. Testing manual COM object creation...
powershell -Command "try { $obj = New-Object -ComObject '{B3F3A1C4-8F9E-4A2D-9B5C-7E6F8D4C9A3B}'; Write-Host '✅ COM object created successfully'; $obj = $null } catch { Write-Host '❌ COM creation failed:' $_.Exception.Message }"

echo.
echo ========================================
echo Debug test completed
echo ========================================
pause