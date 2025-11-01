#include "virtual_camera_filter.h"
#include "directshow_source_filter.h"
#include "../capture/frame.h"
#include <iostream>
#include <windows.h>
#include <dshow.h>
#include <atlbase.h>
#include <comdef.h>

// DirectShow virtual camera implementation
#pragma comment(lib, "strmiids.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")

VirtualCameraFilter::VirtualCameraFilter() 
    : m_initialized(false), m_registered(false), m_running(false), m_pSourceFilter(nullptr) {
    m_deviceName = "MySubstitute Virtual Camera";
}

VirtualCameraFilter::~VirtualCameraFilter() {
    Stop();
    Unregister();
    
    // Release the source filter
    if (m_pSourceFilter) {
        delete m_pSourceFilter;
        m_pSourceFilter = nullptr;
    }
}

bool VirtualCameraFilter::Initialize() {
    if (m_initialized) {
        return true;
    }
    
    // Initialize COM for DirectShow operations
    HRESULT hr = CoInitialize(nullptr);
    if (FAILED(hr)) {
        std::cerr << "Failed to initialize COM for virtual camera" << std::endl;
        return false;
    }
    
    // Create simplified virtual camera filter instance
    m_pSourceFilter = SimpleVirtualCameraFilter::CreateInstance();
    
    if (!m_pSourceFilter) {
        std::cerr << "Failed to create virtual camera filter" << std::endl;
        return false;
    }
    
    if (!m_pSourceFilter->Initialize()) {
        std::cerr << "Failed to initialize virtual camera filter" << std::endl;
        delete m_pSourceFilter;
        m_pSourceFilter = nullptr;
        return false;
    }
    
    std::cout << "Initializing virtual camera filter: " << m_deviceName << std::endl;
    
    m_initialized = true;
    return true;
}

bool VirtualCameraFilter::Register() {
    if (!m_initialized) {
        std::cerr << "Virtual camera filter not initialized" << std::endl;
        return false;
    }
    
    if (m_registered) {
        return true;
    }
    
    // Register virtual camera with system
    if (!m_pSourceFilter->Register()) {
        std::cerr << "Failed to register virtual camera filter" << std::endl;
        return false;
    }
    
    std::cout << "Registered virtual camera filter successfully" << std::endl;
    
    m_registered = true;
    return true;
}

void VirtualCameraFilter::Unregister() {
    if (!m_registered) {
        return;
    }
    
    Stop();
    
    // Unregister virtual camera filter
    if (m_pSourceFilter) {
        m_pSourceFilter->Unregister();
        std::cout << "Unregistered virtual camera filter successfully" << std::endl;
    }
    
    m_registered = false;
}

bool VirtualCameraFilter::Start() {
    if (!m_initialized || !m_registered) {
        std::cerr << "Virtual camera filter not ready to start" << std::endl;
        return false;
    }
    
    if (m_running) {
        return true;
    }
    
    // Start virtual camera filter
    if (!m_pSourceFilter->Start()) {
        std::cerr << "Failed to start virtual camera filter" << std::endl;
        return false;
    }
    
    std::cout << "Starting virtual camera: " << m_deviceName << std::endl;
    
    m_running = true;
    return true;
}

void VirtualCameraFilter::Stop() {
    if (!m_running) {
        return;
    }
    
    // Stop virtual camera filter
    if (m_pSourceFilter) {
        m_pSourceFilter->Stop();
    }
    
    std::cout << "Stopping virtual camera: " << m_deviceName << std::endl;
    
    m_running = false;
}

bool VirtualCameraFilter::IsRunning() const {
    return m_running;
}

void VirtualCameraFilter::UpdateFrame(const Frame& frame) {
    if (!m_running || !m_pSourceFilter) {
        return;
    }
    
    // Push frame to DirectShow filter
    m_pSourceFilter->UpdateFrame(frame);
}

void VirtualCameraFilter::SetFrameSource(std::function<Frame()> callback) {
    m_frameSource = callback;
}

std::string VirtualCameraFilter::GetDeviceName() const {
    return m_deviceName;
}

void VirtualCameraFilter::SetDeviceName(const std::string& name) {
    if (!name.empty()) {
        m_deviceName = name;
    }
}