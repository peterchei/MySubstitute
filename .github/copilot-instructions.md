# MySubstitute - AI Agent Instructions

## Project Overview
MySubstitute is a Windows virtual camera application that intercepts real camera feeds, processes them through AI algorithms, and outputs the processed video as a virtual camera device. Applications like Zoom, Teams, and browsers can then use this virtual camera.

## Architecture Flow
```
Real Camera → CameraCapture → AIProcessor → VirtualCameraFilter → Applications
```

Key pipeline: Physical camera capture → Frame buffering → AI processing (pluggable) → DirectShow virtual camera registration → Application consumption.

## Core Components & Patterns

### 1. Interface-Based Design Pattern
All major components use abstract base classes with factory methods:
- `AIProcessor` base class with concrete `PassthroughProcessor` 
- `CameraCapture` with static `Create()` factory method
- `VirtualCameraFilter` for DirectShow integration
- Common pattern: `Initialize()` → `Start/Process()` → `Stop()` lifecycle

### 2. Frame Processing Pipeline (`src/ai/`)
- `AIProcessor::ProcessFrame(const Frame& input) -> Frame` is the core interface
- Multiple processors can be chained via `AIProcessingPipeline`
- Frame data structure defined in `src/capture/frame.h`
- Always implement both `Initialize()` and `Cleanup()` methods

### 3. COM/DirectShow Integration (`src/capture/`, `src/virtual_camera/`)
- All DirectShow code requires `CoInitialize()`/`CoUninitialize()` bracketing
- Camera enumeration uses DirectShow device enumeration APIs
- Virtual camera registration requires administrator privileges
- Memory management follows COM reference counting patterns

## Build System & Dependencies

### CMake Structure
- Single executable build with conditional OpenCV support
- Uses `HAVE_OPENCV` preprocessor flag for optional features
- Links against Windows multimedia libraries: `ole32`, `strmiids`, `mfplat`, `mf`
- C++17 standard with Windows-specific definitions (`WIN32_LEAN_AND_MEAN`, `NOMINMAX`)

### Development Workflow
1. **Setup**: Run `setup.bat` to verify Visual Studio, CMake, and dependencies
2. **Build**: Use `build.bat` (creates Visual Studio solution in `/build/`)
3. **Test**: Run `run.bat` or execute from `/build/bin/`
4. **Debug**: Visual Studio project files generated in `/build/` directory

### Critical Dependencies
- **Visual Studio 2019/2022** with Desktop C++ workload
- **Windows SDK** with DirectShow base classes (must be built manually)
- **OpenCV 4.x** (optional, but recommended via vcpkg)
- **Administrator privileges** required for virtual camera driver registration

## File Naming & Code Conventions

### Source Organization
- Each component has dedicated folder: `capture/`, `ai/`, `virtual_camera/`, `service/`
- Header/source pairs: `component_name.h` + `component_name.cpp`
- Interface definitions in headers, platform-specific implementations in source
- CMakeLists.txt per component (though currently using single executable)

### Error Handling
- COM methods return `HRESULT`, check with `FAILED()` macro
- C++ exceptions used for high-level errors, COM errors handled directly
- Always cleanup COM objects and release DirectShow interfaces
- Use RAII patterns for resource management

## Key Integration Points

### DirectShow Filter Registration
Virtual camera requires system registration via `regsvr32` or programmatic COM registration. The filter must implement DirectShow source filter interfaces and appear in device enumeration.

### Frame Format Handling
- Input frames from camera capture (various formats: RGB, YUV, etc.)
- AI processing expects consistent format (conversion in capture layer)  
- Output to virtual camera maintains format compatibility
- Frame timing and synchronization critical for smooth video

### Service Integration (`src/service/`)
- Background Windows service for persistent operation
- System tray UI for user interaction
- Service can auto-start and run without user login
- Handles multiple applications accessing virtual camera simultaneously

## Development Gotchas
- **DirectShow base classes**: Must build Windows SDK samples manually before linking
- **COM initialization**: Required before any DirectShow/MediaFoundation calls
- **Admin privileges**: Virtual camera registration and some DirectShow operations require elevation
- **OpenCV linking**: Conditional compilation based on `HAVE_OPENCV` flag
- **Memory leaks**: DirectShow interfaces must be properly released
- **Threading**: Camera capture and AI processing likely need separate threads

## Testing Strategy
Main executable (`src/main.cpp`) provides basic component testing:
- Camera enumeration and device detection
- AI processor initialization and basic frame processing
- Component integration verification
- COM initialization/cleanup validation

Use this for rapid iteration when developing new AI processors or camera capture modifications.