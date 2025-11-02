@echo off
echo ========================================
echo Testing Virtual Camera with DeviceManager
echo ========================================

echo.
echo 1. Listing all video input devices via PowerShell...
powershell -Command "Add-Type -AssemblyName System.Core; [System.Linq.Enumerable]::Range(0, 10) | ForEach-Object { try { $cap = New-Object -ComObject WMPlayer.OCX; Write-Host 'Device' $_ ':' $cap.Cdrom.GetByDriveSpecifier($_) } catch {} }"

echo.
echo 2. Checking DirectShow video capture devices...
powershell -Command "try { $deviceEnum = New-Object -ComObject WbemScripting.SWbemLocator; $service = $deviceEnum.ConnectServer('.', 'root\cimv2'); $devices = $service.ExecQuery('SELECT * FROM Win32_PnPEntity WHERE PNPClass=''Image'' OR Name LIKE ''%%camera%%'''); foreach($device in $devices) { Write-Host 'Found:' $device.Name } } catch { Write-Host 'Error:' $_.Exception.Message }"

echo.
echo 3. Testing with Windows Media Format SDK...
powershell -Command "try { Add-Type -TypeDefinition 'using System; using System.Runtime.InteropServices; public class MediaDevices { [DllImport(\"winmm.dll\")] public static extern int capGetDriverDescription(int wDriverIndex, System.Text.StringBuilder lpszName, int cbName, System.Text.StringBuilder lpszVer, int cbVer); }'; for($i=0; $i -lt 10; $i++) { $name = New-Object System.Text.StringBuilder(80); $ver = New-Object System.Text.StringBuilder(80); $result = [MediaDevices]::capGetDriverDescription($i, $name, 80, $ver, 80); if($result -ne 0) { Write-Host 'Capture Device' $i ':' $name.ToString() '(' $ver.ToString() ')' } } } catch { Write-Host 'Media enumeration not available' }"

echo.
echo ========================================
echo Test completed - check results above
echo ========================================
pause