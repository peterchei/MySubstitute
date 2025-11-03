# Quick Model Setup Script
# Run this AFTER manually downloading generator_Hayao.onnx to your Downloads folder

$downloadsPath = "$env:USERPROFILE\Downloads\generator_Hayao.onnx"
$targetPath = "C:\Users\peter\git\MySubstitute\models\anime_gan.onnx"

Write-Host "Checking for downloaded model..." -ForegroundColor Cyan

if (Test-Path $downloadsPath) {
    Write-Host "✓ Found model in Downloads folder" -ForegroundColor Green
    
    # Move and rename
    Move-Item $downloadsPath $targetPath -Force
    
    if (Test-Path $targetPath) {
        $fileSize = (Get-Item $targetPath).Length / 1MB
        Write-Host "✓ Model installed successfully!" -ForegroundColor Green
        Write-Host "  Location: $targetPath" -ForegroundColor Gray
        Write-Host "  Size: $([math]::Round($fileSize, 2)) MB" -ForegroundColor Gray
        Write-Host ""
        Write-Host "You can now run MySubstitute and select 'Anime GAN (AI - GPU)'" -ForegroundColor Cyan
    } else {
        Write-Host "✗ Failed to move file" -ForegroundColor Red
    }
} else {
    Write-Host "✗ Model not found in Downloads folder" -ForegroundColor Red
    Write-Host ""
    Write-Host "Please download it manually:" -ForegroundColor Yellow
    Write-Host "1. Open: https://huggingface.co/bryandlee/animegan2-pytorch/resolve/main/generator_Hayao.onnx" -ForegroundColor Gray
    Write-Host "2. Wait for download to complete (8.5 MB)" -ForegroundColor Gray
    Write-Host "3. Run this script again" -ForegroundColor Gray
}
