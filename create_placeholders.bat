@echo off
REM Create placeholder images for PersonReplacementProcessor
REM Run this once to generate default target images

echo ========================================
echo Creating Placeholder Images
echo ========================================
echo.

REM Check if Python is available
python --version >nul 2>&1
if errorlevel 1 (
    echo ERROR: Python is not installed or not in PATH
    echo Please install Python 3.x from https://www.python.org/
    pause
    exit /b 1
)

REM Check if OpenCV is installed
python -c "import cv2" >nul 2>&1
if errorlevel 1 (
    echo OpenCV not found. Installing...
    pip install opencv-python numpy
    if errorlevel 1 (
        echo ERROR: Failed to install OpenCV
        pause
        exit /b 1
    )
)

REM Run the placeholder creation script
echo Running placeholder creation script...
python create_placeholder_images.py

if errorlevel 1 (
    echo.
    echo ERROR: Failed to create placeholder images
    pause
    exit /b 1
)

echo.
echo ========================================
echo SUCCESS!
echo ========================================
echo.
echo Placeholder images created in assets/ folder:
echo   - assets/default_face.jpg (for face swap)
echo   - assets/default_person.jpg (for full body replacement)
echo.
echo You can now use the Person Replacement filters!
echo.
echo TIP: Replace these placeholder images with your own photos
echo      for better replacement results.
echo.
pause
