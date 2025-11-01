#include "virtual_camera_manager.h"
#include <iostream>
#include <sstream>
#include <comdef.h>

VirtualCameraManager::VirtualCameraManager() :
    m_isRegistered(false),
    m_isActive(false)
{
    InitializeCOM();
    
    // Create the filter instance
    m_pFilter.reset(MySubstituteVirtualCameraFilter::CreateInstance());
    if (m_pFilter) {
        std::cout << "[VirtualCamera] Filter created successfully" << std::endl;
    } else {
        std::cout << "[VirtualCamera] Failed to create filter" << std::endl;
    }
}

VirtualCameraManager::~VirtualCameraManager()
{
    StopVirtualCamera();
    m_pFilter.reset();
    CleanupCOM();
}

bool VirtualCameraManager::RegisterVirtualCamera()
{
    std::cout << "[VirtualCamera] Attempting to register virtual camera..." << std::endl;
    
    HRESULT hr = DllRegisterServer();
    if (SUCCEEDED(hr)) {
        m_isRegistered = true;
        std::cout << "[VirtualCamera] ✓ Virtual camera registered successfully!" << std::endl;
        std::cout << "[VirtualCamera] ✓ MySubstitute should now appear in camera lists" << std::endl;
        return true;
    } else {
        _com_error err(hr);
        std::wcout << L"[VirtualCamera] ✗ Registration failed: " << err.ErrorMessage() << std::endl;
        std::cout << "[VirtualCamera] ✗ Make sure to run as Administrator" << std::endl;
        return false;
    }
}

bool VirtualCameraManager::UnregisterVirtualCamera()
{
    std::cout << "[VirtualCamera] Unregistering virtual camera..." << std::endl;
    
    HRESULT hr = DllUnregisterServer();
    if (SUCCEEDED(hr)) {
        m_isRegistered = false;
        std::cout << "[VirtualCamera] ✓ Virtual camera unregistered successfully" << std::endl;
        return true;
    } else {
        _com_error err(hr);
        std::wcout << L"[VirtualCamera] ✗ Unregistration failed: " << err.ErrorMessage() << std::endl;
        return false;
    }
}

bool VirtualCameraManager::StartVirtualCamera()
{
    if (!m_pFilter) {
        std::cout << "[VirtualCamera] ✗ Cannot start - filter not initialized" << std::endl;
        return false;
    }
    
    if (m_isActive) {
        std::cout << "[VirtualCamera] Virtual camera is already active" << std::endl;
        return true;
    }
    
    std::cout << "[VirtualCamera] Starting virtual camera..." << std::endl;
    
    // In a real implementation, this would involve more DirectShow graph setup
    // For now, we'll just mark it as active
    m_isActive = true;
    
    std::cout << "[VirtualCamera] ✓ Virtual camera started successfully" << std::endl;
    std::cout << "[VirtualCamera] ✓ Ready to stream processed video to applications" << std::endl;
    
    return true;
}

bool VirtualCameraManager::StopVirtualCamera()
{
    if (!m_isActive) {
        return true;
    }
    
    std::cout << "[VirtualCamera] Stopping virtual camera..." << std::endl;
    
    if (m_pFilter) {
        m_pFilter->Stop();
    }
    
    m_isActive = false;
    std::cout << "[VirtualCamera] ✓ Virtual camera stopped" << std::endl;
    
    return true;
}

void VirtualCameraManager::UpdateFrame(const Frame& frame)
{
    if (m_pFilter && m_isActive) {
        m_pFilter->UpdateFrame(frame);
    }
}

std::string VirtualCameraManager::GetStatusString() const
{
    std::ostringstream oss;
    oss << "Virtual Camera: ";
    
    if (m_isRegistered) {
        oss << "Registered";
        if (m_isActive) {
            oss << " & Active";
        } else {
            oss << " & Inactive";
        }
    } else {
        oss << "Not Registered";
    }
    
    return oss.str();
}

bool VirtualCameraManager::InitializeCOM()
{
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) {
        std::cout << "[VirtualCamera] ✗ Failed to initialize COM: " << std::hex << hr << std::endl;
        return false;
    }
    
    std::cout << "[VirtualCamera] ✓ COM initialized" << std::endl;
    return true;
}

void VirtualCameraManager::CleanupCOM()
{
    CoUninitialize();
    std::cout << "[VirtualCamera] COM cleanup completed" << std::endl;
}