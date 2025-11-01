#include "camera_capture.h"
#include "frame.h"
#include <iostream>

CameraCapture::CameraCapture() 
    : m_initialized(false), m_capturing(false), m_selectedDevice(-1) {
}

CameraCapture::~CameraCapture() {
    StopCapture();
}

bool CameraCapture::Initialize() {
    if (m_initialized) {
        return true;
    }
    
    // Platform-specific initialization will be handled in derived classes
    m_initialized = true;
    return true;
}

bool CameraCapture::StartCapture() {
    if (!m_initialized) {
        std::cerr << "Camera capture not initialized" << std::endl;
        return false;
    }
    
    if (m_capturing) {
        return true;
    }
    
    if (m_selectedDevice < 0) {
        std::cerr << "No camera selected" << std::endl;
        return false;
    }
    
    // Platform-specific capture start will be implemented in derived classes
    m_capturing = true;
    return true;
}

void CameraCapture::StopCapture() {
    if (!m_capturing) {
        return;
    }
    
    // Platform-specific capture stop will be implemented in derived classes
    m_capturing = false;
}

bool CameraCapture::IsCapturing() const {
    return m_capturing;
}

std::vector<CameraDevice> CameraCapture::GetAvailableCameras() {
    // Default implementation returns empty list
    // Platform-specific implementations will override this
    return std::vector<CameraDevice>();
}

bool CameraCapture::SelectCamera(int deviceId) {
    if (deviceId < 0) {
        return false;
    }
    
    auto cameras = GetAvailableCameras();
    for (const auto& camera : cameras) {
        if (camera.id == deviceId && camera.isAvailable) {
            m_selectedDevice = deviceId;
            return true;
        }
    }
    
    return false;
}

void CameraCapture::SetFrameCallback(std::function<void(const Frame&)> callback) {
    m_frameCallback = callback;
}

bool CameraCapture::SetFrameRate(int fps) {
    if (fps <= 0 || fps > 120) {
        return false;
    }
    
    // Implementation depends on platform-specific capture method
    return true;
}

bool CameraCapture::SetResolution(int width, int height) {
    if (width <= 0 || height <= 0) {
        return false;
    }
    
    // Implementation depends on platform-specific capture method
    return true;
}