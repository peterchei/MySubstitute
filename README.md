# MySubstitute - Virtual Camera with AI Video Processing

A Windows virtual camera application that captures real camera feeds, processes them through AI algorithms, and provides live preview with professional overlay features. Built with C++, OpenCV, and DirectShow.

## ✨ Features

### 🎥 **Live Camera Integration**
- Real-time camera capture via OpenCV and DirectShow
- Automatic camera detection and enumeration
- 30 FPS smooth video processing
- Thread-safe multi-camera support

### 🤖 **AI Processing Pipeline**
- Pluggable AI processor architecture  
- Professional caption overlays with semi-transparent backgrounds
- Customizable text positioning, scaling, and colors
- Real-time frame processing with minimal latency

### 📱 **Live Preview System**
- Mobile phone-sized preview window (270x480)
- Real-time display of processed video feed
- Always-on-top and positioning controls
- Right-click context menu for quick settings

### 🎛️ **System Tray Controls**
- Background operation with system tray integration
- Camera start/stop controls
- Live status monitoring and tooltips
- Quick access to preview and settings

## Architecture

```
Physical Camera → OpenCV Capture → AI Processing → Live Preview
      ↓              ↓                ↓             ↓
  DirectShow → Frame Capture → Caption Overlay → Mobile Window
  Enumeration     (30 FPS)        (Real-time)      (270x480)
```

## 🚀 **Current Implementation Status**

### ✅ **Fully Implemented**
- **Real Camera Capture**: OpenCV-based capture with DirectShow enumeration
- **Live Video Processing**: 30 FPS real-time AI processing with caption overlays
- **Mobile Preview Window**: Professional 270x480 preview with live video feed
- **System Tray Integration**: Complete background operation with menu controls
- **Caption Filter**: Professional text overlays with semi-transparent backgrounds
- **Thread-Safe Pipeline**: Multi-threaded capture and processing system

### 🔧 **Core Components**

#### **1. Camera Capture System (`src/capture/`)**
- `DirectShowCameraCapture`: OpenCV-based camera access
- `Frame`: Thread-safe frame data structure with OpenCV Mat integration
- Multi-camera enumeration via DirectShow API
- Background capture thread with 30 FPS frame rate control

#### **2. AI Processing Pipeline (`src/ai/`)**
- `AIProcessor`: Abstract base class for pluggable processors
- `PassthroughProcessor`: Caption overlay with timestamp and watermark support
- Professional text rendering with semi-transparent backgrounds
- Real-time frame processing with minimal latency

#### **3. Live Preview System (`src/ui/`)**
- `PreviewWindowManager`: Mobile phone-sized video preview (270x480)
- `SystemTrayManager`: Background operation with context menu controls
- Real-time video rendering with Windows GDI+ 
- Always-on-top, positioning, and right-click context menus

## 🛠️ **Technical Requirements**

### **Development Environment**
- **OS**: Windows 10/11
- **IDE**: Visual Studio 2019/2022 with Desktop C++ workload
- **Build System**: CMake 3.16+ with Visual Studio generator
- **C++ Standard**: C++17

### **Dependencies**
- **OpenCV 4.x**: Computer vision and camera capture
- **DirectShow**: Camera enumeration and Windows media integration
- **Windows SDK**: System tray, windowing, and COM APIs

## 📦 **Build & Run**

### **Quick Start**
```bash
# 1. Check system requirements
setup.bat

# 2. Build the application
build.bat

# 3. Run MySubstitute
run.bat
```

### **Setup & Installation**
```bash
# Verify system requirements and dependencies
setup.bat

# Build application (generates Visual Studio solution)
build.bat

# Launch MySubstitute with live camera integration
run.bat
```

## 🎥 **How to Use**

### **Starting MySubstitute**
1. **Launch Application**: Run `MySubstitute.exe` or use `run.bat`
2. **System Tray**: Application runs in background with tray icon
3. **Camera Access**: Right-click tray icon → "Select Camera" to choose input device

### **Live Video Experience**
1. **Select Camera**: Choose from available cameras via tray menu
2. **Live Preview**: Mobile-style preview window appears automatically
3. **Video Processing**: Real-time caption overlay with timestamp
4. **Background Operation**: Continues running until explicitly closed

### **Controls & Features**
- **Tray Menu**: Right-click for camera selection and application controls
- **Preview Window**: Right-click for positioning and display options
- **Live Captions**: Professional text overlay with transparent background
- **Multi-Camera**: Switch between cameras without restart

## 🏗️ **Architecture Overview**

```
┌─────────────────┐    ┌──────────────────┐    ┌─────────────────┐
│  Real Camera    │───▶│  AI Processing   │───▶│  Live Preview   │
│  (OpenCV)       │    │  (Captions)      │    │  (Mobile View)  │
└─────────────────┘    └──────────────────┘    └─────────────────┘
         │                        │                        │
         ▼                        ▼                        ▼
┌─────────────────┐    ┌──────────────────┐    ┌─────────────────┐
│  Frame Capture  │    │  Frame Buffer    │    │  System Tray    │
│  (Background)   │    │  (Thread-Safe)   │    │  (Controls)     │
└─────────────────┘    └──────────────────┘    └─────────────────┘
```

## 🔧 **Development**

### **Project Structure**
```
src/
├── main.cpp              # WinMain entry point with GUI message loop
├── capture/              # Camera capture system
│   ├── camera_capture.*  # DirectShow + OpenCV camera access
│   └── frame.*          # Thread-safe frame data structures
├── ai/                   # Processing pipeline
│   ├── ai_processor.*   # Abstract processor interface
│   └── passthrough_processor.*  # Caption and overlay processor
└── ui/                   # User interface components
    ├── system_tray_manager.*    # Background tray integration
    └── preview_window_manager.* # Live video preview window
```

### **Adding New Processing Features**
1. **Inherit AIProcessor**: Create new class extending `AIProcessor`
2. **Implement ProcessFrame**: Add your frame transformation logic
3. **Register in Main**: Add processor to the pipeline in `main.cpp`
4. **Test Live**: Use preview window for real-time testing

## 📋 **Development Status**

### ✅ **Completed Features**
- Real camera capture and enumeration
- Live video processing with caption overlays
- Mobile-style preview window with real-time display
- System tray integration with camera controls
- Thread-safe frame processing pipeline
- Professional text rendering with transparency

### 🚧 **Future Enhancements**
- Virtual camera driver for application integration
- Advanced AI filters (background replacement, face effects)
- Windows service for always-on operation
- Configuration UI for processing parameters

## 📄 **License**

[To be determined]