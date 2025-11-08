# Download MediaPipe Selfie Segmentation Model for Virtual Background
# This model provides professional-quality person segmentation like Google Meet

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  MediaPipe Selfie Segmentation Model" -ForegroundColor Cyan
Write-Host "  Professional Virtual Background" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

$modelsDir = "models"
$modelFile = "selfie_segmentation.tflite"
$modelPath = Join-Path $modelsDir $modelFile

# Create models directory if it doesn't exist
if (!(Test-Path $modelsDir)) {
    Write-Host "Creating models directory..." -ForegroundColor Yellow
    New-Item -ItemType Directory -Path $modelsDir | Out-Null
}

# Check if model already exists
if (Test-Path $modelPath) {
    Write-Host "✅ Model already exists: $modelPath" -ForegroundColor Green
    $size = (Get-Item $modelPath).Length / 1MB
    Write-Host "   File size: $([math]::Round($size, 2)) MB" -ForegroundColor Gray
    
    $response = Read-Host "   Do you want to re-download? (y/N)"
    if ($response -ne "y" -and $response -ne "Y") {
        Write-Host ""
        Write-Host "Skipping download. Using existing model." -ForegroundColor Green
        Write-Host ""
        Write-Host "Next steps:" -ForegroundColor Cyan
        Write-Host "  1. The model is ready to use" -ForegroundColor Gray
        Write-Host "  2. Rebuild MySubstitute if needed" -ForegroundColor Gray
        Write-Host "  3. Virtual Background will automatically use this model" -ForegroundColor Gray
        exit 0
    }
    
    Write-Host "Removing existing model..." -ForegroundColor Yellow
    Remove-Item $modelPath -Force
}

# Download URLs for different model versions
$modelUrls = @{
    "General" = "https://storage.googleapis.com/mediapipe-models/image_segmenter/selfie_segmenter/float16/latest/selfie_segmenter.tflite"
    "Landscape" = "https://storage.googleapis.com/mediapipe-models/image_segmenter/selfie_segmentation_landscape/float16/latest/selfie_segmentation_landscape.tflite"
}

Write-Host ""
Write-Host "Available models:" -ForegroundColor Cyan
Write-Host "  1) General (256x256) - Best for video calls, fastest" -ForegroundColor White
Write-Host "  2) Landscape (144x256) - Optimized for wide shots" -ForegroundColor White
Write-Host ""

$choice = Read-Host "Select model (1 or 2, default: 1)"
if ([string]::IsNullOrWhiteSpace($choice)) {
    $choice = "1"
}

$modelUrl = ""
$modelType = ""
switch ($choice) {
    "1" {
        $modelUrl = $modelUrls["General"]
        $modelType = "General (256x256)"
        $modelFile = "selfie_segmentation.tflite"
    }
    "2" {
        $modelUrl = $modelUrls["Landscape"]
        $modelType = "Landscape (144x256)"
        $modelFile = "selfie_segmentation_landscape.tflite"
    }
    default {
        $modelUrl = $modelUrls["General"]
        $modelType = "General (256x256)"
        $modelFile = "selfie_segmentation.tflite"
    }
}

$modelPath = Join-Path $modelsDir $modelFile

Write-Host ""
Write-Host "Downloading $modelType model..." -ForegroundColor Yellow
Write-Host "URL: $modelUrl" -ForegroundColor Gray
Write-Host "Destination: $modelPath" -ForegroundColor Gray
Write-Host ""

try {
    # Download with progress
    $ProgressPreference = 'SilentlyContinue'
    Invoke-WebRequest -Uri $modelUrl -OutFile $modelPath -UseBasicParsing
    $ProgressPreference = 'Continue'
    
    if (Test-Path $modelPath) {
        $size = (Get-Item $modelPath).Length / 1MB
        Write-Host "✅ Download complete!" -ForegroundColor Green
        Write-Host "   File: $modelPath" -ForegroundColor Gray
        Write-Host "   Size: $([math]::Round($size, 2)) MB" -ForegroundColor Gray
        Write-Host ""
        
        # Display model information
        Write-Host "Model Information:" -ForegroundColor Cyan
        Write-Host "  Type: MediaPipe Selfie Segmentation" -ForegroundColor White
        Write-Host "  Format: TFLite (TensorFlow Lite)" -ForegroundColor White
        Write-Host "  Precision: FP16 (half-precision)" -ForegroundColor White
        Write-Host "  Input Size: $(if ($choice -eq "2") { "144x256" } else { "256x256" })" -ForegroundColor White
        Write-Host "  Expected Performance:" -ForegroundColor White
        Write-Host "    • CPU: 15-20 FPS" -ForegroundColor Gray
        Write-Host "    • GPU: 60-90 FPS (with ONNX Runtime + DirectML/CUDA)" -ForegroundColor Gray
        Write-Host ""
        
        # Check if we should convert to ONNX
        Write-Host "⚠️  Note: TFLite format downloaded" -ForegroundColor Yellow
        Write-Host ""
        Write-Host "For best performance on Windows, convert to ONNX format:" -ForegroundColor Cyan
        Write-Host "  1. Install Python and tf2onnx:" -ForegroundColor Gray
        Write-Host "     pip install tf2onnx tensorflow onnx" -ForegroundColor White
        Write-Host ""
        Write-Host "  2. Convert model:" -ForegroundColor Gray
        Write-Host "     python -m tf2onnx.convert --tflite $modelPath --output models/selfie_segmentation.onnx" -ForegroundColor White
        Write-Host ""
        Write-Host "Or use the TFLite model directly (slower but works without conversion)" -ForegroundColor Gray
        Write-Host ""
        
        Write-Host "Next Steps:" -ForegroundColor Cyan
        Write-Host "  1. ✅ Model downloaded and ready" -ForegroundColor White
        Write-Host "  2. Optional: Convert to ONNX for better performance" -ForegroundColor White
        Write-Host "  3. Install ONNX Runtime (see docs/GPU_ACCELERATION.md)" -ForegroundColor White
        Write-Host "  4. Rebuild MySubstitute with -DUSE_ONNX=ON" -ForegroundColor White
        Write-Host "  5. Enable virtual background in application" -ForegroundColor White
        Write-Host ""
        Write-Host "The virtual background processor will automatically detect and use this model!" -ForegroundColor Green
        Write-Host ""
        
    } else {
        Write-Host "❌ Download failed - file not found" -ForegroundColor Red
        exit 1
    }
    
} catch {
    Write-Host "❌ Download failed!" -ForegroundColor Red
    Write-Host "Error: $($_.Exception.Message)" -ForegroundColor Red
    Write-Host ""
    Write-Host "Troubleshooting:" -ForegroundColor Yellow
    Write-Host "  1. Check internet connection" -ForegroundColor Gray
    Write-Host "  2. Try manual download from:" -ForegroundColor Gray
    Write-Host "     $modelUrl" -ForegroundColor White
    Write-Host "  3. Save to: $modelPath" -ForegroundColor White
    exit 1
}

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  Download Complete!" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Cyan
