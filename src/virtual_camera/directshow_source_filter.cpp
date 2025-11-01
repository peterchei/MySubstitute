#include "directshow_source_filter.h"
#include <iostream>

//
// SimpleVirtualCameraFilter implementation
//

SimpleVirtualCameraFilter::SimpleVirtualCameraFilter()
    : m_initialized(false), m_registered(false), m_running(false)
{
}

SimpleVirtualCameraFilter::~SimpleVirtualCameraFilter()
{
    Stop();
    Unregister();
}

SimpleVirtualCameraFilter* SimpleVirtualCameraFilter::CreateInstance()
{
    return new SimpleVirtualCameraFilter();
}

bool SimpleVirtualCameraFilter::Initialize()
{
    if (m_initialized) {
        return true;
    }
    
    std::cout << "SimpleVirtualCamera: Initializing (placeholder implementation)" << std::endl;
    
    m_initialized = true;
    return true;
}

bool SimpleVirtualCameraFilter::Register()
{
    if (!m_initialized) {
        std::cerr << "SimpleVirtualCamera: Not initialized" << std::endl;
        return false;
    }
    
    if (m_registered) {
        return true;
    }
    
    std::cout << "SimpleVirtualCamera: Registering as virtual camera device (placeholder)" << std::endl;
    std::cout << "NOTE: Full DirectShow registration requires DirectShow base classes" << std::endl;
    
    m_registered = true;
    return true;
}

void SimpleVirtualCameraFilter::Unregister()
{
    if (!m_registered) {
        return;
    }
    
    Stop();
    
    std::cout << "SimpleVirtualCamera: Unregistering virtual camera device" << std::endl;
    
    m_registered = false;
}

bool SimpleVirtualCameraFilter::Start()
{
    if (!m_initialized || !m_registered) {
        std::cerr << "SimpleVirtualCamera: Not ready to start" << std::endl;
        return false;
    }
    
    if (m_running) {
        return true;
    }
    
    std::cout << "SimpleVirtualCamera: Starting virtual camera output" << std::endl;
    
    m_running = true;
    return true;
}

void SimpleVirtualCameraFilter::Stop()
{
    if (!m_running) {
        return;
    }
    
    std::cout << "SimpleVirtualCamera: Stopping virtual camera output" << std::endl;
    
    m_running = false;
}

bool SimpleVirtualCameraFilter::IsRunning() const
{
    return m_running;
}

void SimpleVirtualCameraFilter::UpdateFrame(const Frame& frame)
{
    if (!m_running) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(m_frameMutex);
    m_latestFrame = frame;
    
    // In a real implementation, this would push the frame to DirectShow output pin
    if (frame.IsValid()) {
        // Just track that we're receiving frames for now
        static int frameCount = 0;
        frameCount++;
        if (frameCount % 60 == 0) { // Log every 2 seconds at 30fps
            std::cout << "SimpleVirtualCamera: Received " << frameCount << " frames" << std::endl;
        }
    }
}

Frame SimpleVirtualCameraFilter::GetLatestFrame()
{
    std::lock_guard<std::mutex> lock(m_frameMutex);
    return m_latestFrame;
}

std::string SimpleVirtualCameraFilter::GetStatusMessage() const
{
    if (!m_initialized) {
        return "Virtual camera not initialized";
    }
    if (!m_registered) {
        return "Virtual camera not registered with system";
    }
    if (!m_running) {
        return "Virtual camera registered but not running";
    }
    return "Virtual camera running (placeholder - not visible to applications yet)";
}

// All implementation moved to SimpleVirtualCameraFilter class above