@echo off
echo ================================================
echo  MySubstitute - AI Model Setup
echo ================================================
echo.
echo This will download professional AI models for:
echo  - Face Swapping (InsightFace inswapper_128.onnx)
echo  - Smooth, realistic face replacement
echo.
echo Download size: ~128 MB
echo.
pause

python scripts\download_insightface_model.py

echo.
echo ================================================
echo  Setup Complete!
echo ================================================
echo.
echo AI models are now available in the models\ folder.
echo Rebuild the application to use them:
echo   1. build.bat
echo   2. run.bat
echo   3. Select "AI Face Swap" filter
echo.
pause
