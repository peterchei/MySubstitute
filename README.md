# MySubstitute - Virtual Camera with AI Video Processing

A Windows virtual camera application that allows any camera-using application to receive AI-processed video instead of the real camera feed.

## Project Overview

MySubstitute creates a virtual camera device on Windows that:
1. Captures video from the real camera
2. Processes the video through AI algorithms (customizable)
3. Outputs the processed video as a virtual camera device
4. Allows any application to use this virtual camera instead of the real one

## Architecture

```
Real Camera → Capture Module → AI Processing → Virtual Camera → Applications
                    ↓               ↓              ↓
                Frame Buffer → Enhancement → Output Buffer → DirectShow Filter
```

## Components

### 1. Camera Capture Module (`src/capture/`)
- DirectShow/MediaFoundation integration
- Real camera access and frame capture
- Frame format conversion and buffering

### 2. AI Processing Pipeline (`src/ai/`)
- Pluggable AI processing interface
- Frame transformation and enhancement
- Background replacement, face filters, etc.

### 3. Virtual Camera Driver (`src/virtual_camera/`)
- DirectShow virtual camera filter
- Frame injection and streaming
- Device enumeration and compatibility

### 4. Background Service (`src/service/`)
- Windows service implementation
- Continuous operation and monitoring
- System tray interface

### 5. Configuration UI (`src/ui/`)
- Settings and preferences
- Camera selection and AI options
- Real-time preview and controls

## Technical Requirements

- **OS**: Windows 10/11
- **Framework**: C++ with DirectShow/MediaFoundation
- **AI**: OpenCV for image processing, extensible for ML models
- **UI**: Qt or Windows native APIs
- **Service**: Windows Service API

## Build Requirements

- Visual Studio 2019/2022
- Windows SDK
- DirectShow SDK
- OpenCV
- Qt (for UI)

## Installation

1. Install the virtual camera driver (requires admin privileges)
2. Run the background service
3. Configure settings through the UI
4. Select "MySubstitute Virtual Camera" in any application

## Development Status

🚧 **In Development** 🚧

This project is currently in the planning and initial development phase.

## License

[To be determined]