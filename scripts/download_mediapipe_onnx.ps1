# Download MediaPipe Selfie Segmentation Model (ONNX format)
# Converts the model to ONNX format for use with ONNX Runtime

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  MediaPipe Selfie Segmentation (ONNX)" -ForegroundColor Cyan
Write-Host "  Professional Virtual Background" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

$modelsDir = "models"
$onnxModel = "selfie_segmentation_mediapipe.onnx"
$onnxPath = Join-Path $modelsDir $onnxModel

# Create models directory if it doesn't exist
if (!(Test-Path $modelsDir)) {
    Write-Host "Creating models directory..." -ForegroundColor Yellow
    New-Item -ItemType Directory -Path $modelsDir | Out-Null
}

# Check if model already exists
if (Test-Path $onnxPath) {
    Write-Host "✅ ONNX model already exists: $onnxPath" -ForegroundColor Green
    $size = (Get-Item $onnxPath).Length / 1MB
    Write-Host "   File size: $([math]::Round($size, 2)) MB" -ForegroundColor Gray
    Write-Host ""
    Write-Host "Model is ready to use!" -ForegroundColor Green
    exit 0
}

# Download pre-converted ONNX model from Hugging Face
Write-Host "Downloading MediaPipe Selfie Segmentation (ONNX)..." -ForegroundColor Cyan
Write-Host "Source: Hugging Face Model Hub" -ForegroundColor Gray
Write-Host ""

$url = "https://huggingface.co/PINTO/selfie_segmentation/resolve/main/selfie_segmentation_256x256.onnx"

try {
    Write-Host "Downloading from: $url" -ForegroundColor Gray
    $ProgressPreference = 'SilentlyContinue'
    Invoke-WebRequest -Uri $url -OutFile $onnxPath -UseBasicParsing
    $ProgressPreference = 'Continue'
    
    if (Test-Path $onnxPath) {
        $size = (Get-Item $onnxPath).Length / 1MB
        Write-Host ""
        Write-Host "✅ Download complete!" -ForegroundColor Green
        Write-Host "   File: $onnxPath" -ForegroundColor Gray
        Write-Host "   Size: $([math]::Round($size, 2)) MB" -ForegroundColor Gray
        Write-Host ""
        Write-Host "Next steps:" -ForegroundColor Cyan
        Write-Host "  1. Rebuild MySubstitute: build.bat" -ForegroundColor White
        Write-Host "  2. Run with debug mode: run_debug.bat" -ForegroundColor White
        Write-Host "  3. Select 'ONNX (MediaPipe)' from dropdown" -ForegroundColor White
        Write-Host ""
        Write-Host "Expected performance:" -ForegroundColor Cyan
        Write-Host "  - CPU: ~30 FPS" -ForegroundColor Gray
        Write-Host "  - GPU: ~60-90 FPS (with ONNX Runtime GPU)" -ForegroundColor Gray
    } else {
        throw "Download failed - file not created"
    }
} catch {
    Write-Host ""
    Write-Host "❌ Download failed: $_" -ForegroundColor Red
    Write-Host ""
    Write-Host "Alternative: Manual Download" -ForegroundColor Yellow
    Write-Host "  1. Visit: https://huggingface.co/PINTO/selfie_segmentation" -ForegroundColor Gray
    Write-Host "  2. Download: selfie_segmentation_256x256.onnx" -ForegroundColor Gray
    Write-Host "  3. Save to: models/selfie_segmentation_mediapipe.onnx" -ForegroundColor Gray
    exit 1
}
