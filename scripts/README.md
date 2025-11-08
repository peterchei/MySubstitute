# MySubstitute Scripts

Helper scripts and test files for development and setup.

## 📂 Contents

### 🔧 Setup & Download Scripts

#### Model Download Scripts
- **`download_segmentation_model.ps1`** - Download MediaPipe Selfie Segmentation models
  - Interactive menu for General (256x256) or Landscape (144x256) models
  - Auto-detects existing models
  - Provides conversion instructions
  
- **`download_anime_model.ps1`** - Download AnimeGAN style transfer models
  - Legacy script for anime style models
  
- **`download_compatible_models.ps1`** - Download various compatible AI models
  - Multiple model types support
  
- **`setup_model.ps1`** - Setup script for model configuration
  - Helper for model initialization

#### Build & Deploy Scripts
- **`copy_dll_safe.ps1`** - Safely copy DirectShow DLL with COM unregistration
  - Handles locked DLLs
  - Automatic COM server registration/unregistration

### 🧪 Test Scripts

#### Console Test Scripts
- **`test_filters_console.ps1`** - Test all AI filters from console
  - Quick filter validation
  
- **`test_virtual_background.bat`** - Test virtual background functionality
  - Batch script for quick testing

#### Test Executables (C++ source)
- **`test_anime_gpu.cpp`** - Test GPU acceleration for anime filters
  - Validates CUDA/OpenCV DNN backends
  
- **`test_face_filter.cpp`** - Test face detection and filtering
  - Face filter validation
  
- **`test_filter_callback.cpp`** - Test filter callback mechanism
  - Event system testing

### 🛠️ Build Configuration Files
- **`CMakeLists_DirectShow.txt`** - Alternative DirectShow build configuration
- **`CMakeLists_test_face_filter.txt`** - Face filter test build config

### 🔨 Utility Scripts
- **`create_placeholders.cpp`** - Create placeholder files for testing
  - Development utility

## 📝 Usage Examples

### Download Segmentation Model (Recommended)
```powershell
.\scripts\download_segmentation_model.ps1
```
Interactive menu will guide you through downloading the best model for virtual backgrounds.

### Test Virtual Background
```batch
.\scripts\test_virtual_background.bat
```
Quick test of background removal functionality.

### Test Filters from Console
```powershell
.\scripts\test_filters_console.ps1
```
Run through all available AI filters for validation.

### Copy DLL Safely
```powershell
.\scripts\copy_dll_safe.ps1
```
Used automatically during build process to handle COM registration.

## 🎯 Common Tasks

### I want to...

**...download models for virtual background**
```powershell
.\scripts\download_segmentation_model.ps1
```

**...test if filters are working**
```powershell
.\scripts\test_filters_console.ps1
```

**...test GPU acceleration**
```powershell
# Build test executable first
cmake --build build --config Debug --target test_anime_gpu
# Then run
.\build\bin\Debug\test_anime_gpu.exe
```

**...validate face detection**
```powershell
# Build test executable first
cmake --build build --config Debug --target test_face_filter
# Then run
.\build\bin\Debug\test_face_filter.exe
```

## 🔄 Script Dependencies

### PowerShell Scripts
- Require PowerShell 5.1 or later
- Some scripts may require internet connection (download scripts)
- May require administrator privileges (DLL registration)

### Batch Scripts
- Standard Windows batch files
- No special requirements

### C++ Test Files
- Require compilation via CMake
- Build configuration defined in main CMakeLists.txt
- Produce executables in `build/bin/` directory

## 📋 Test Executables

The C++ test files are compiled into standalone executables:

| Source File | Executable | Purpose |
|-------------|------------|---------|
| test_anime_gpu.cpp | test_anime_gpu.exe | GPU acceleration testing |
| test_face_filter.cpp | test_face_filter.exe | Face detection validation |
| test_filter_callback.cpp | test_filter_callback.exe | Callback system testing |

Build them with:
```powershell
cmake --build build --config Debug
```

They will appear in: `build/bin/Debug/`

## 🚀 Development Workflow

1. **Setup Models** - Run download scripts to get AI models
2. **Build Project** - Use main build system
3. **Test Filters** - Run test scripts to validate
4. **Deploy DLL** - Copy script handles registration

## ⚠️ Notes

- **Administrator Rights**: Some scripts (especially DLL copy) may require admin privileges
- **Internet Required**: Download scripts need active internet connection
- **Build First**: Test executables must be built before running
- **Path Dependency**: Scripts expect to be run from repository root

## 📚 Related Documentation

- [Virtual Background Setup](../docs/VIRTUAL_BACKGROUND_SETUP.md) - Model setup guide
- [Development Setup](../docs/development_setup.md) - Complete dev environment
- [GPU Acceleration](../docs/GPU_ACCELERATION.md) - GPU setup details

---

**Tip**: Run scripts from the repository root directory for correct path resolution.
