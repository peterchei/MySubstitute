# Download OpenCV DNN Compatible Style Transfer Models
# These models are verified to work with OpenCV DNN module

param(
    [string]$Style = "all"
)

$modelsDir = "models"
if (-not (Test-Path $modelsDir)) {
    New-Item -ItemType Directory -Path $modelsDir | Out-Null
}

Write-Host "Downloading OpenCV DNN compatible models..." -ForegroundColor Green

# Fast Neural Style Transfer models (Torch format - WORKS WITH OPENCV!)
$models = @{
    "candy" = "https://cs.stanford.edu/people/jcjohns/fast-neural-style/models/instance_norm/candy.t7"
    "mosaic" = "https://cs.stanford.edu/people/jcjohns/fast-neural-style/models/instance_norm/mosaic.t7"
    "udnie" = "https://cs.stanford.edu/people/jcjohns/fast-neural-style/models/instance_norm/udnie.t7"
    "the_scream" = "https://cs.stanford.edu/people/jcjohns/fast-neural-style/models/instance_norm/the_scream.t7"
    "feathers" = "https://cs.stanford.edu/people/jcjohns/fast-neural-style/models/instance_norm/feathers.t7"
}

if ($Style -eq "all") {
    foreach ($name in $models.Keys) {
        $url = $models[$name]
        $output = Join-Path $modelsDir "style_$name.t7"
        
        Write-Host "`nDownloading $name style..." -ForegroundColor Cyan
        try {
            Invoke-WebRequest -Uri $url -OutFile $output -UseBasicParsing
            $size = (Get-Item $output).Length / 1MB
            Write-Host "✓ Downloaded $name ($([math]::Round($size, 2)) MB)" -ForegroundColor Green
        } catch {
            Write-Host "✗ Failed to download $name : $_" -ForegroundColor Red
        }
    }
} else {
    if ($models.ContainsKey($Style)) {
        $url = $models[$Style]
        $output = Join-Path $modelsDir "style_$Style.t7"
        
        Write-Host "`nDownloading $Style style..." -ForegroundColor Cyan
        try {
            Invoke-WebRequest -Uri $url -OutFile $output -UseBasicParsing
            $size = (Get-Item $output).Length / 1MB
            Write-Host "✓ Downloaded $Style ($([math]::Round($size, 2)) MB)" -ForegroundColor Green
        } catch {
            Write-Host "✗ Failed to download $Style : $_" -ForegroundColor Red
        }
    } else {
        Write-Host "Unknown style: $Style" -ForegroundColor Red
        Write-Host "Available styles: $($models.Keys -join ', ')" -ForegroundColor Yellow
    }
}

Write-Host "`n✓ Download complete!" -ForegroundColor Green
Write-Host "`nTo use these models, you'll need to implement a Style Transfer processor" -ForegroundColor Yellow
Write-Host "that loads .t7 (Torch) models using cv::dnn::readNetFromTorch()" -ForegroundColor Yellow

# Display summary
Write-Host "`n=== Downloaded Models ===" -ForegroundColor Cyan
Get-ChildItem -Path $modelsDir -Filter "*.t7" | ForEach-Object {
    $sizeMB = [math]::Round($_.Length / 1MB, 2)
    Write-Host "$($_.Name) - $sizeMB MB" -ForegroundColor White
}
