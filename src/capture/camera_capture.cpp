#include "camera_capture.h"
#include "frame.h"
#include <iostream>
#include <vector>

#ifdef HAVE_OPENCV
#include <opencv2/opencv.hpp>
#endif

#include <windows.h>
#include <dshow.h>
#include <comutil.h>

#pragma comment(lib, "strmiids.lib")

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
    std::vector<CameraDevice> devices;
    
    // Initialize COM if not already initialized
    HRESULT hr = CoInitialize(nullptr);
    bool comInitialized = SUCCEEDED(hr);
    
    // Try to enumerate using DirectShow
    ICreateDevEnum* pDevEnum = nullptr;
    IEnumMoniker* pEnum = nullptr;
    
    hr = CoCreateInstance(CLSID_SystemDeviceEnum, nullptr, CLSCTX_INPROC_SERVER, 
                          IID_ICreateDevEnum, (void**)&pDevEnum);
    
    if (SUCCEEDED(hr)) {
        hr = pDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEnum, 0);
        
        if (SUCCEEDED(hr) && pEnum) {
            IMoniker* pMoniker = nullptr;
            int deviceIndex = 0;
            
            while (pEnum->Next(1, &pMoniker, nullptr) == S_OK) {
                IPropertyBag* pPropBag = nullptr;
                hr = pMoniker->BindToStorage(nullptr, nullptr, IID_IPropertyBag, (void**)&pPropBag);
                
                if (SUCCEEDED(hr)) {
                    VARIANT var;
                    VariantInit(&var);
                    
                    hr = pPropBag->Read(L"Description", &var, nullptr);
                    if (FAILED(hr)) {
                        hr = pPropBag->Read(L"FriendlyName", &var, nullptr);
                    }
                    
                    if (SUCCEEDED(hr)) {
                        // Convert BSTR to std::string
                        std::string name;
                        if (var.bstrVal) {
                            int len = WideCharToMultiByte(CP_UTF8, 0, var.bstrVal, -1, nullptr, 0, nullptr, nullptr);
                            if (len > 0) {
                                std::vector<char> buffer(len);
                                WideCharToMultiByte(CP_UTF8, 0, var.bstrVal, -1, buffer.data(), len, nullptr, nullptr);
                                name = buffer.data();
                            }
                        }
                        
                        if (!name.empty()) {
                            CameraDevice device(deviceIndex++, name);
                            device.isAvailable = true;
                            devices.push_back(device);
                        }
                    }
                    
                    VariantClear(&var);
                    pPropBag->Release();
                }
                
                pMoniker->Release();
            }
            
            pEnum->Release();
        }
        
        pDevEnum->Release();
    }
    
    // If no devices found, add dummy devices for testing
    if (devices.empty()) {
        devices.push_back(CameraDevice(0, "Default Camera (simulated)"));
        devices.push_back(CameraDevice(1, "Secondary Camera (simulated)"));
    }
    
    if (comInitialized) {
        CoUninitialize();
    }
    
    return devices;
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

std::unique_ptr<CameraCapture> CameraCapture::Create() {
    return std::make_unique<CameraCapture>();
}