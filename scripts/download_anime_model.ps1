# AnimeGAN Model Downloader
# This script downloads the AnimeGANv2 ONNX model and sets it up for MySubstitute

param(
    [Parameter(Mandatory=$false)]
    [ValidateSet("Hayao", "Shinkai", "Paprika")]
    [string]$Style = "Hayao"
)

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  AnimeGAN Model Downloader" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# Determine project root (where this script is located)
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$projectRoot = $scriptDir
$modelsDir = Join-Path $projectRoot "models"
$modelPath = Join-Path $modelsDir "anime_gan.onnx"

Write-Host "Project Root: $projectRoot" -ForegroundColor Yellow
Write-Host "Models Directory: $modelsDir" -ForegroundColor Yellow
Write-Host ""

# Model URLs
$modelUrls = @{
    "Hayao" = @{
        "url" = "https://huggingface.co/bryandlee/animegan2-pytorch/resolve/main/generator_Hayao.onnx"
        "description" = "Hayao Miyazaki style - general anime look, most popular"
        "size" = "~8.5 MB"
    }
    "Shinkai" = @{
        "url" = "https://huggingface.co/bryandlee/animegan2-pytorch/resolve/main/generator_Shinkai.onnx"
        "description" = "Makoto Shinkai style - more realistic anime"
        "size" = "~8.5 MB"
    }
    "Paprika" = @{
        "url" = "https://huggingface.co/bryandlee/animegan2-pytorch/resolve/main/generator_Paprika.onnx"
        "description" = "Paprika style - vibrant colors"
        "size" = "~8.5 MB"
    }
}

$selectedModel = $modelUrls[$Style]
Write-Host "Selected Style: $Style" -ForegroundColor Green
Write-Host "Description: $($selectedModel.description)" -ForegroundColor Gray
Write-Host "Size: $($selectedModel.size)" -ForegroundColor Gray
Write-Host ""

# Check if models directory exists
if (-not (Test-Path $modelsDir)) {
    Write-Host "Creating models directory..." -ForegroundColor Yellow
    New-Item -ItemType Directory -Path $modelsDir -Force | Out-Null
    Write-Host "✓ Created: $modelsDir" -ForegroundColor Green
} else {
    Write-Host "✓ Models directory already exists" -ForegroundColor Green
}
Write-Host ""

# Check if model already exists
if (Test-Path $modelPath) {
    $existingSize = (Get-Item $modelPath).Length / 1MB
    Write-Host "⚠ Model already exists at: $modelPath" -ForegroundColor Yellow
    Write-Host "  Current size: $([math]::Round($existingSize, 2)) MB" -ForegroundColor Gray
    Write-Host ""
    
    $response = Read-Host "Do you want to overwrite it? (y/n)"
    if ($response -ne "y") {
        Write-Host "Download cancelled." -ForegroundColor Yellow
        exit 0
    }
    Write-Host ""
}

# Download the model
Write-Host "Downloading $Style model..." -ForegroundColor Cyan
Write-Host "URL: $($selectedModel.url)" -ForegroundColor Gray
Write-Host ""

try {
    $ProgressPreference = 'SilentlyContinue'  # Faster download
    Invoke-WebRequest -Uri $selectedModel.url -OutFile $modelPath -UseBasicParsing
    $ProgressPreference = 'Continue'
    
    # Verify download
    if (Test-Path $modelPath) {
        $fileSize = (Get-Item $modelPath).Length / 1MB
        Write-Host ""
        Write-Host "========================================" -ForegroundColor Green
        Write-Host "  Download Successful!" -ForegroundColor Green
        Write-Host "========================================" -ForegroundColor Green
        Write-Host ""
        Write-Host "✓ Model saved to: $modelPath" -ForegroundColor Green
        Write-Host "✓ File size: $([math]::Round($fileSize, 2)) MB" -ForegroundColor Green
        Write-Host ""
        
        # Check file size reasonableness
        if ($fileSize -lt 1) {
            Write-Host "⚠ Warning: File size is very small ($([math]::Round($fileSize, 2)) MB)" -ForegroundColor Yellow
            Write-Host "  The download may have failed. Try again." -ForegroundColor Yellow
            Write-Host ""
        } elseif ($fileSize -gt 100) {
            Write-Host "⚠ Warning: File size is very large ($([math]::Round($fileSize, 2)) MB)" -ForegroundColor Yellow
            Write-Host "  This may not be the correct model file." -ForegroundColor Yellow
            Write-Host ""
        }
        
        Write-Host "Next Steps:" -ForegroundColor Cyan
        Write-Host "1. Build MySubstitute: cmake --build build --config Release" -ForegroundColor Gray
        Write-Host "2. Run: .\build\bin\Release\MySubstitute.exe" -ForegroundColor Gray
        Write-Host "3. Select 'Anime GAN (AI - GPU)' from the filter dropdown" -ForegroundColor Gray
        Write-Host ""
        Write-Host "For detailed setup and troubleshooting:" -ForegroundColor Cyan
        Write-Host "  See: ANIME_GAN_QUICKSTART.md" -ForegroundColor Gray
        Write-Host "  Or:  docs/anime_gan_setup.md" -ForegroundColor Gray
        Write-Host ""
        
    } else {
        throw "File not found after download"
    }
    
} catch {
    Write-Host ""
    Write-Host "========================================" -ForegroundColor Red
    Write-Host "  Download Failed!" -ForegroundColor Red
    Write-Host "========================================" -ForegroundColor Red
    Write-Host ""
    Write-Host "Error: $_" -ForegroundColor Red
    Write-Host ""
    Write-Host "Manual Download:" -ForegroundColor Yellow
    Write-Host "1. Open: $($selectedModel.url)" -ForegroundColor Gray
    Write-Host "2. Save as: $modelPath" -ForegroundColor Gray
    Write-Host ""
    exit 1
}

Write-Host "Done!" -ForegroundColor Green
