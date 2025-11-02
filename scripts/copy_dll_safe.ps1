# Safe DLL copy script that handles COM registration
param(
    [string]$SourceDLL,
    [string]$TargetDLL
)

Write-Host "Copying DLL safely: $SourceDLL -> $TargetDLL"

# Check if target exists and is registered
if (Test-Path $TargetDLL) {
    Write-Host "Target DLL exists, attempting to unregister..."
    try {
        # Try to unregister (requires admin, but we'll ignore failures)
        $process = Start-Process "regsvr32.exe" -ArgumentList "/u", "/s", "`"$TargetDLL`"" -Wait -PassThru -WindowStyle Hidden
        if ($process.ExitCode -eq 0) {
            Write-Host "Successfully unregistered DLL"
            # Give Windows time to release the DLL
            Start-Sleep -Milliseconds 500
        } else {
            Write-Host "Unregistration failed (may not have been registered)"
        }
    }
    catch {
        Write-Host "Unregistration skipped: $($_.Exception.Message)"
    }
}

# Copy the file
try {
    Copy-Item $SourceDLL $TargetDLL -Force
    Write-Host "DLL copied successfully"
}
catch {
    Write-Host "Warning: Could not copy DLL - $($_.Exception.Message)"
    Write-Host "This usually means the DLL is in use by a COM registration."
    Write-Host "To fix: Run as Administrator or unregister the virtual camera first."
    # Don't exit with error - just warn
}

Write-Host "Copy operation completed"