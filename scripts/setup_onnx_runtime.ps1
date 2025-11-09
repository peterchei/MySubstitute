# Setup ONNX Runtime for Virtual Background
# Downloads and installs ONNX Runtime (CPU version - no CUDA required)

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  ONNX Runtime Setup" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

$destPath = "D:\DevTools\onnxruntime-win-x64-1.16.3"

# Check if already installed
if (Test-Path $destPath) {
    Write-Host "✅ ONNX Runtime already installed at:" -ForegroundColor Green
    Write-Host "   $destPath" -ForegroundColor Gray
    Write-Host ""
    Write-Host "Next step: Rebuild MySubstitute" -ForegroundColor Cyan
    Write-Host "  cmake -B build -DUSE_ONNX=ON" -ForegroundColor White
    Write-Host "  cmake --build build --config Debug" -ForegroundColor White
    exit 0
}

# Create parent directory
$parentDir = Split-Path $destPath -Parent
if (!(Test-Path $parentDir)) {
    Write-Host "Creating directory: $parentDir" -ForegroundColor Yellow
    New-Item -ItemType Directory -Path $parentDir -Force | Out-Null
}

Write-Host "Downloading ONNX Runtime (CPU version)..." -ForegroundColor Cyan
Write-Host "Version: 1.16.3" -ForegroundColor Gray
Write-Host "Size: ~50 MB" -ForegroundColor Gray
Write-Host ""

$url = "https://github.com/microsoft/onnxruntime/releases/download/v1.16.3/onnxruntime-win-x64-1.16.3.zip"
$zipFile = "$env:TEMP\onnxruntime.zip"

try {
    $ProgressPreference = 'SilentlyContinue'
    Invoke-WebRequest -Uri $url -OutFile $zipFile -UseBasicParsing
    $ProgressPreference = 'Continue'
    
    Write-Host "Extracting..." -ForegroundColor Cyan
    Expand-Archive -Path $zipFile -DestinationPath $parentDir -Force
    
    if (Test-Path $destPath) {
        Write-Host ""
        Write-Host "✅ ONNX Runtime installed successfully!" -ForegroundColor Green
        Write-Host "   Location: $destPath" -ForegroundColor Gray
        Write-Host ""
        Write-Host "Next steps:" -ForegroundColor Cyan
        Write-Host "  1. Update CMake configuration:" -ForegroundColor White
        Write-Host "     cmake -B build -DUSE_ONNX=ON" -ForegroundColor Gray
        Write-Host ""
        Write-Host "  2. Rebuild MySubstitute:" -ForegroundColor White
        Write-Host "     cmake --build build --config Debug" -ForegroundColor Gray
        Write-Host ""
        Write-Host "  3. Run and test:" -ForegroundColor White
        Write-Host "     run_debug.bat" -ForegroundColor Gray
        
        # Cleanup
        Remove-Item $zipFile -Force -ErrorAction SilentlyContinue
    } else {
        throw "Installation failed - directory not created"
    }
} catch {
    Write-Host ""
    Write-Host "❌ Download/extraction failed: $_" -ForegroundColor Red
    Write-Host ""
    Write-Host "Manual installation:" -ForegroundColor Yellow
    Write-Host "  1. Download: $url" -ForegroundColor Gray
    Write-Host "  2. Extract to: $parentDir" -ForegroundColor Gray
    exit 1
}
