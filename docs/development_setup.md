# MySubstitute Development Setup Guide

## Prerequisites

### Required Software
1. **Visual Studio 2019/2022** (Community Edition or higher)
   - Workloads: "Desktop development with C++"
   - Individual components: "Windows 10/11 SDK (latest version)"

2. **CMake 3.20 or higher**
   - Download from https://cmake.org/download/
   - Add to PATH during installation

3. **Git** (for version control)
   - Download from https://git-scm.com/

### Required Libraries

#### 1. Windows SDK with DirectShow Base Classes
- Install Windows 10/11 SDK with samples
- Location: `C:\Program Files (x86)\Windows Kits\10\Samples\multimedia\directshow\baseclasses`
- **Important**: You need to build the base classes manually (see instructions below)

#### 2. OpenCV 4.x
```powershell
# Option 1: Download pre-built binaries
# https://opencv.org/releases/ - download Windows version
# Extract to C:\opencv

# Option 2: Use vcpkg (recommended)
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg integrate install
.\vcpkg install opencv[contrib]:x64-windows
```

#### 3. Qt 6.x (Optional - for UI)
```powershell
# Download Qt6 installer from https://www.qt.io/download-qt-installer
# Or use vcpkg:
.\vcpkg install qt6-base:x64-windows
.\vcpkg install qt6-widgets:x64-windows
```

## Build Instructions

### Step 1: Clone and Setup
```powershell
cd C:\Users\peter\git
git clone <your-repo-url> MySubstitute  # Or use existing folder
cd MySubstitute
mkdir build
cd build
```

### Step 2: Build DirectShow Base Classes (REQUIRED)
```powershell
# Navigate to DirectShow base classes
cd "C:\Program Files (x86)\Windows Kits\10\Samples\multimedia\directshow\baseclasses"

# Open Visual Studio Developer Command Prompt (as Administrator)
# Build for both Debug and Release
nmake -f baseclasses.mak CFG=retail
nmake -f baseclasses.mak CFG=debug
```

### Step 3: Configure CMake
```powershell
cd C:\Users\peter\git\MySubstitute\build

# If using vcpkg
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake

# If OpenCV is installed manually
cmake .. -DOpenCV_DIR=C:\opencv\build

# If DirectShow base classes are in non-standard location
cmake .. -DDIRECTSHOW_INCLUDE_DIR="C:\path\to\baseclasses"
```

### Step 4: Build Project
```powershell
# Build Debug version
cmake --build . --config Debug

# Build Release version  
cmake --build . --config Release
```

## Development Workflow

### Running the Test Application
```powershell
cd build\bin\Debug
.\MySubstitute.exe
```

### Project Structure
```
MySubstitute/
├── src/
│   ├── capture/           # Camera capture functionality
│   ├── ai/               # AI processing modules  
│   ├── virtual_camera/   # DirectShow virtual camera
│   ├── service/          # Background service
│   ├── ui/              # User interface (Qt)
│   └── main.cpp         # Test application
├── docs/                # Documentation
├── build/               # Build output
└── CMakeLists.txt       # Build configuration
```

### Adding New AI Processors
1. Create new class inheriting from `AIProcessor`
2. Implement all virtual methods
3. Add to `AIProcessorFactory::CreateProcessor()`
4. Update CMakeLists.txt if needed

### Debugging Tips
1. Use Visual Studio debugger with the generated .vcxproj files
2. Enable DirectShow debug output: Set `HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\DirectX\SDK\DirectShow\Debug` = 1
3. Use GraphEdit tool to test DirectShow filters
4. Windows Event Viewer for service debugging

## Common Issues

### Issue: DirectShow Base Classes Not Found
**Solution**: 
- Ensure Windows SDK with samples is installed
- Build the base classes manually using nmake
- Check DIRECTSHOW_INCLUDE_DIR path in CMake

### Issue: OpenCV Not Found
**Solution**:
- Set OpenCV_DIR to the correct path
- Use vcpkg for automatic dependency management
- Verify PATH includes OpenCV DLL directories

### Issue: Virtual Camera Not Registering
**Solution**:
- Run registration as Administrator
- Use `regsvr32` to register the DirectShow filter
- Check Windows registry entries

### Issue: Permission Denied for Service Operations
**Solution**:
- Run Visual Studio as Administrator for debugging services
- Use separate elevated process for service operations
- Test with console application first

## Testing Applications

Test the virtual camera with these applications:
- **Windows Camera app**
- **Google Chrome** (chrome://settings/content/camera)
- **Zoom** (Settings → Video)
- **Microsoft Teams** (Settings → Devices → Camera)
- **OBS Studio** (Sources → Video Capture Device)

## Next Steps

1. Complete the DirectShow filter implementation
2. Add Windows-specific camera capture using DirectShow/MediaFoundation
3. Implement the virtual camera registration system
4. Create the system tray application
5. Add comprehensive error handling and logging

## Resources

- [DirectShow Programming Guide](https://docs.microsoft.com/en-us/windows/win32/directshow/directshow-programming-guide)
- [OpenCV Documentation](https://docs.opencv.org/)
- [Qt Documentation](https://doc.qt.io/)
- [Windows Service Programming](https://docs.microsoft.com/en-us/windows/win32/services/services)