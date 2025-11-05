# Test Virtual Background Filters with Console Output
Write-Host "Starting MySubstitute with console output..." -ForegroundColor Green
Write-Host "Please test the Virtual Background filters (indexes 2-5) in the UI" -ForegroundColor Yellow
Write-Host "Watch this console for debug output" -ForegroundColor Yellow
Write-Host ""

cd build\bin\Debug
.\MySubstitute_d.exe
