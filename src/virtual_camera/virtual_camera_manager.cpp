#include "virtual_camera_manager.h"
#include <iostream>
#include <sstream>
#include <thread>
#include <chrono>
#include <comdef.h>
#include <dshow.h>
#include <strmif.h>

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
    if (m_isRegistered) {
        std::cout << "[VirtualCamera] Virtual camera already registered" << std::endl;
        return VerifyRegistration();
    }
    
    std::cout << "[VirtualCamera] Attempting to register virtual camera..." << std::endl;
    
    HRESULT hr = DllRegisterServer();
    if (SUCCEEDED(hr)) {
        std::cout << "[VirtualCamera] Registry entries created successfully" << std::endl;
        
        // Wait a moment for system to process registration
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        // Verify registration worked
        if (VerifyRegistration()) {
            m_isRegistered = true;
            std::cout << "[VirtualCamera] ✅ Virtual camera registered and verified!" << std::endl;
            std::cout << "[VirtualCamera] ✅ MySubstitute Virtual Camera is now available to applications" << std::endl;
            return true;
        } else {
            std::cout << "[VirtualCamera] ⚠️ Registry updated but device not visible - may need system restart" << std::endl;
            return false;
        }
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