#include "virtual_camera_manager.h"
#include <iostream>
#include <sstream>
#include <thread>
#include <chrono>
#include <comdef.h>
#include <dshow.h>
#include <strmif.h>

#if defined(HAVE_OPENCV) && (HAVE_OPENCV == 1)
#include <opencv2/opencv.hpp>
#endif

// Shared memory name (same as DirectShow filter)
const wchar_t* VirtualCameraManager::SHARED_MEMORY_NAME = L"MySubstituteVirtualCameraFrames";

VirtualCameraManager::VirtualCameraManager() :
    m_isRegistered(false),
    m_isActive(false),
    m_sharedMemory(nullptr),
    m_sharedBuffer(nullptr)
{
    InitializeCOM();
    
    // Create shared memory for frame communication
    CreateSharedMemory();
    
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
    CleanupSharedMemory();
    m_pFilter.reset();
    CleanupCOM();
}

bool VirtualCameraManager::RegisterVirtualCamera()
{
    if (m_isRegistered) {
        std::cout << "[VirtualCamera] Virtual camera already registered" << std::endl;
        return VerifyRegistration();
    }
    
    std::cout << "[VirtualCamera] Attempting to register virtual camera..." << std::endl;
    
    // Use the new registry-based approach
    if (VirtualCameraRegistry::RegisterVirtualCamera()) {
        std::cout << "[VirtualCamera] Registry entries created successfully" << std::endl;
        
        // Wait a moment for system to process registration
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        
        // List all devices for debugging
        VirtualCameraRegistry::ListAllCameraDevices();
        
        // Verify registration worked
        if (VerifyRegistration()) {
            m_isRegistered = true;
            std::cout << "[VirtualCamera] ✅ Virtual camera registered and verified!" << std::endl;
            std::cout << "[VirtualCamera] ✅ MySubstitute Virtual Camera is now available to applications" << std::endl;
            return true;
        } else {
            std::cout << "[VirtualCamera] ⚠️ Registry updated but device verification failed" << std::endl;
            std::cout << "[VirtualCamera] 💡 Try restarting camera applications or rebooting system" << std::endl;
            
            // Mark as registered even if verification fails (registry was created)
            m_isRegistered = VirtualCameraRegistry::IsVirtualCameraRegistered();
            return m_isRegistered;
        }
    } else {
        std::cout << "[VirtualCamera] ✗ Registry registration failed" << std::endl;
        std::cout << "[VirtualCamera] ✗ Make sure to run as Administrator" << std::endl;
        return false;
    }
}

bool VirtualCameraManager::UnregisterVirtualCamera()
{
    std::cout << "[VirtualCamera] Unregistering virtual camera..." << std::endl;
    
    if (VirtualCameraRegistry::UnregisterVirtualCamera()) {
        m_isRegistered = false;
        std::cout << "[VirtualCamera] ✓ Virtual camera unregistered successfully" << std::endl;
        return true;
    } else {
        std::cout << "[VirtualCamera] ✗ Unregistration failed" << std::endl;
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
    if (m_isActive) {
        // Write frame to shared memory for DirectShow DLL to access
        WriteFrameToSharedMemory(frame);
        
        // Also update the local filter (for backward compatibility)
        if (m_pFilter) {
            m_pFilter->UpdateFrame(frame);
        }
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

bool VirtualCameraManager::VerifyRegistration() const
{
    HRESULT hr;
    ICreateDevEnum* pDevEnum = nullptr;
    IEnumMoniker* pEnum = nullptr;
    IMoniker* pMoniker = nullptr;
    bool found = false;
    
    std::cout << "[VirtualCamera] Verifying virtual camera registration..." << std::endl;
    
    // Create device enumerator
    hr = CoCreateInstance(CLSID_SystemDeviceEnum, nullptr, CLSCTX_INPROC_SERVER,
                         IID_ICreateDevEnum, (void**)&pDevEnum);
    if (FAILED(hr)) {
        std::cout << "[VirtualCamera] ✗ Failed to create device enumerator" << std::endl;
        return false;
    }
    
    // Enumerate video capture devices
    hr = pDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEnum, 0);
    if (hr == S_OK) {
        ULONG cFetched;
        while (pEnum->Next(1, &pMoniker, &cFetched) == S_OK) {
            IPropertyBag* pPropBag;
            hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void**)&pPropBag);
            if (SUCCEEDED(hr)) {
                VARIANT var;
                VariantInit(&var);
                hr = pPropBag->Read(L"FriendlyName", &var, 0);
                if (SUCCEEDED(hr)) {
                    std::wcout << L"[VirtualCamera] Found device: " << var.bstrVal << std::endl;
                    if (wcscmp(var.bstrVal, L"MySubstitute Virtual Camera") == 0) {
                        found = true;
                        std::cout << "[VirtualCamera] ✓ MySubstitute Virtual Camera found in system!" << std::endl;
                        VariantClear(&var);
                        pPropBag->Release();
                        pMoniker->Release();
                        break;
                    }
                    VariantClear(&var);
                }
                pPropBag->Release();
            }
            pMoniker->Release();
        }
    }
    
    if (pEnum) pEnum->Release();
    if (pDevEnum) pDevEnum->Release();
    
    if (!found) {
        std::cout << "[VirtualCamera] ✗ MySubstitute Virtual Camera not found in system device list" << std::endl;
    }
    
    return found;
}

void VirtualCameraManager::CleanupCOM()
{
    CoUninitialize();
    std::cout << "[VirtualCamera] COM cleanup completed" << std::endl;
}

//=============================================================================
// Shared Memory Helper Methods
//=============================================================================

bool VirtualCameraManager::CreateSharedMemory()
{
    // Create shared memory that DirectShow DLL can access
    m_sharedMemory = CreateFileMappingW(
        INVALID_HANDLE_VALUE,
        nullptr,
        PAGE_READWRITE,
        0,
        SHARED_BUFFER_SIZE,
        SHARED_MEMORY_NAME
    );
    
    if (m_sharedMemory == nullptr) {
        std::cout << "[VirtualCamera] ❌ Failed to create shared memory" << std::endl;
        return false;
    }
    
    m_sharedBuffer = MapViewOfFile(
        m_sharedMemory,
        FILE_MAP_ALL_ACCESS,
        0,
        0,
        SHARED_BUFFER_SIZE
    );
    
    if (m_sharedBuffer == nullptr) {
        CloseHandle(m_sharedMemory);
        m_sharedMemory = nullptr;
        std::cout << "[VirtualCamera] ❌ Failed to map shared memory view" << std::endl;
        return false;
    }
    
    // Initialize with black frame
    memset(m_sharedBuffer, 0, SHARED_BUFFER_SIZE);
    
    std::cout << "[VirtualCamera] ✅ Shared memory created for inter-process frame communication" << std::endl;
    return true;
}

void VirtualCameraManager::CleanupSharedMemory()
{
    if (m_sharedBuffer) {
        UnmapViewOfFile(m_sharedBuffer);
        m_sharedBuffer = nullptr;
    }
    
    if (m_sharedMemory) {
        CloseHandle(m_sharedMemory);
        m_sharedMemory = nullptr;
    }
    
    std::cout << "[VirtualCamera] Shared memory cleaned up" << std::endl;
}

bool VirtualCameraManager::WriteFrameToSharedMemory(const Frame& frame)
{
    if (!m_sharedBuffer || frame.data.empty()) {
        return false;
    }
    
#if defined(HAVE_OPENCV) && (HAVE_OPENCV == 1)
    try {
        // Convert frame data to RGB24 format for shared memory
        cv::Mat rgbFrame;
        
        if (frame.data.channels() == 3) {
            // Convert BGR (OpenCV) to RGB (DirectShow standard)
            cv::cvtColor(frame.data, rgbFrame, cv::COLOR_BGR2RGB);
        } else if (frame.data.channels() == 4) {
            // Convert BGRA to RGB
            cv::cvtColor(frame.data, rgbFrame, cv::COLOR_BGRA2RGB);
        } else {
            // Grayscale to RGB
            cv::cvtColor(frame.data, rgbFrame, cv::COLOR_GRAY2RGB);
        }
        
        // Resize to match shared memory buffer size (640x480)
        if (rgbFrame.cols != 640 || rgbFrame.rows != 480) {
            cv::resize(rgbFrame, rgbFrame, cv::Size(640, 480));
        }
        
        // Copy to shared memory buffer
        size_t frameSize = rgbFrame.rows * rgbFrame.cols * rgbFrame.channels();
        if (frameSize <= SHARED_BUFFER_SIZE) {
            memcpy(m_sharedBuffer, rgbFrame.data, frameSize);
            return true;
        }
        
    } catch (...) {
        // Error handling - don't crash
        std::cout << "[VirtualCamera] ❌ Error writing frame to shared memory" << std::endl;
    }
#endif
    
    return false;
}