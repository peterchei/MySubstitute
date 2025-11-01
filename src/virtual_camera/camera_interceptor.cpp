#include "camera_interceptor.h"
#include <dshow.h>
#include <strmif.h>
#include <vector>
#include <iostream>
#include <comdef.h>

CameraInterceptor::CameraInterceptor()
    : m_isIntercepting(false)
    , m_pGraph(nullptr)
    , m_pCapture(nullptr)
    , m_pControl(nullptr)
    , m_pCameraFilter(nullptr)
    , m_pProcessingFilter(nullptr)
{
}

CameraInterceptor::~CameraInterceptor()
{
    ReleaseCamera();
}

std::vector<std::wstring> CameraInterceptor::GetAvailableCameras() /* static */
{
    std::vector<std::wstring> cameras;
    
    HRESULT hr = CoInitialize(nullptr);
    
    ICreateDevEnum* pDevEnum = nullptr;
    hr = CoCreateInstance(CLSID_SystemDeviceEnum, nullptr, CLSCTX_INPROC_SERVER,
                         IID_ICreateDevEnum, (void**)&pDevEnum);
    
    if (SUCCEEDED(hr)) {
        IEnumMoniker* pEnum = nullptr;
        hr = pDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEnum, 0);
        
        if (hr == S_OK) {
            IMoniker* pMoniker = nullptr;
            ULONG cFetched;
            
            while (pEnum->Next(1, &pMoniker, &cFetched) == S_OK) {
                IPropertyBag* pPropBag;
                hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void**)&pPropBag);
                
                if (SUCCEEDED(hr)) {
                    VARIANT var;
                    VariantInit(&var);
                    hr = pPropBag->Read(L"FriendlyName", &var, 0);
                    
                    if (SUCCEEDED(hr)) {
                        cameras.push_back(var.bstrVal);
                        VariantClear(&var);
                    }
                    pPropBag->Release();
                }
                pMoniker->Release();
            }
            pEnum->Release();
        }
        pDevEnum->Release();
    }
    
    CoUninitialize();
    return cameras;
}

bool CameraInterceptor::InterceptCamera(int cameraIndex)
{
    std::wcout << L"[CameraInterceptor] Intercepting camera..." << std::endl;
    
    ReleaseCamera();
    
    HRESULT hr = CoInitialize(nullptr);
    if (FAILED(hr)) return false;
    
    // Create the filter graph manager
    hr = CoCreateInstance(CLSID_FilterGraph, nullptr, CLSCTX_INPROC_SERVER,
                         IID_IGraphBuilder, (void**)&m_pGraph);
    if (FAILED(hr)) return false;
    
    // Create the capture graph builder
    hr = CoCreateInstance(CLSID_CaptureGraphBuilder2, nullptr, CLSCTX_INPROC_SERVER,
                         IID_ICaptureGraphBuilder2, (void**)&m_pCapture);
    if (FAILED(hr)) return false;
    
    // Set the graph
    hr = m_pCapture->SetFiltergraph(m_pGraph);
    if (FAILED(hr)) return false;
    
    // Find camera
    ICreateDevEnum* pDevEnum = nullptr;
    hr = CoCreateInstance(CLSID_SystemDeviceEnum, nullptr, CLSCTX_INPROC_SERVER,
                         IID_ICreateDevEnum, (void**)&pDevEnum);
    
    if (SUCCEEDED(hr)) {
        IEnumMoniker* pEnum = nullptr;
        hr = pDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEnum, 0);
        
        if (hr == S_OK) {
            IMoniker* pMoniker = nullptr;
            ULONG cFetched;
            int currentIndex = 0;
            
            while (pEnum->Next(1, &pMoniker, &cFetched) == S_OK) {
                if (cameraIndex == -1 || currentIndex == cameraIndex) {
                    // Bind to the camera
                    hr = pMoniker->BindToObject(0, 0, IID_IBaseFilter, (void**)&m_pCameraFilter);
                    
                    if (SUCCEEDED(hr)) {
                        // Get camera name
                        IPropertyBag* pPropBag;
                        hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void**)&pPropBag);
                        
                        if (SUCCEEDED(hr)) {
                            VARIANT var;
                            VariantInit(&var);
                            hr = pPropBag->Read(L"FriendlyName", &var, 0);
                            
                            if (SUCCEEDED(hr)) {
                                m_cameraName = var.bstrVal;
                                std::wcout << L"[CameraInterceptor] 📹 Intercepted: " << m_cameraName << std::endl;
                                VariantClear(&var);
                            }
                            pPropBag->Release();
                        }
                        
                        // Add to graph
                        hr = m_pGraph->AddFilter(m_pCameraFilter, L"Camera Filter");
                        if (SUCCEEDED(hr)) {
                            m_isIntercepting = true;
                            pMoniker->Release();
                            break;
                        }
                    }
                }
                
                pMoniker->Release();
                currentIndex++;
            }
            pEnum->Release();
        }
        pDevEnum->Release();
    }
    
    return m_isIntercepting;
}

void CameraInterceptor::ReleaseCamera()
{
    if (m_pControl) {
        m_pControl->Stop();
        m_pControl->Release();
        m_pControl = nullptr;
    }
    
    if (m_pProcessingFilter) {
        m_pProcessingFilter->Release();
        m_pProcessingFilter = nullptr;
    }
    
    if (m_pCameraFilter) {
        m_pCameraFilter->Release();
        m_pCameraFilter = nullptr;
    }
    
    if (m_pCapture) {
        m_pCapture->Release();
        m_pCapture = nullptr;
    }
    
    if (m_pGraph) {
        m_pGraph->Release();
        m_pGraph = nullptr;
    }
    
    m_isIntercepting = false;
    m_cameraName.clear();
    
    std::wcout << L"[CameraInterceptor] ✅ Camera released" << std::endl;
}

bool CameraInterceptor::StartProcessing()
{
    if (!m_isIntercepting || !m_pGraph) return false;
    
    HRESULT hr = m_pGraph->QueryInterface(IID_IMediaControl, (void**)&m_pControl);
    if (FAILED(hr)) return false;
    
    hr = m_pControl->Run();
    if (SUCCEEDED(hr)) {
        std::wcout << L"[CameraInterceptor] ▶️ Started processing: " << m_cameraName << std::endl;
        return true;
    }
    
    return false;
}

void CameraInterceptor::StopProcessing()
{
    if (m_pControl) {
        m_pControl->Stop();
        std::wcout << L"[CameraInterceptor] ⏸️ Stopped processing" << std::endl;
    }
}

// SimpleCameraManager implementation
std::vector<CameraInfo> SimpleCameraManager::ListAllCameras()
{
    std::vector<CameraInfo> cameras;
    
    HRESULT hr = CoInitialize(nullptr);
    
    ICreateDevEnum* pDevEnum = nullptr;
    hr = CoCreateInstance(CLSID_SystemDeviceEnum, nullptr, CLSCTX_INPROC_SERVER,
                         IID_ICreateDevEnum, (void**)&pDevEnum);
    
    if (SUCCEEDED(hr)) {
        IEnumMoniker* pEnum = nullptr;
        hr = pDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEnum, 0);
        
        if (hr == S_OK) {
            IMoniker* pMoniker = nullptr;
            ULONG cFetched;
            
            while (pEnum->Next(1, &pMoniker, &cFetched) == S_OK) {
                IPropertyBag* pPropBag;
                hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void**)&pPropBag);
                
                if (SUCCEEDED(hr)) {
                    VARIANT var;
                    VariantInit(&var);
                    hr = pPropBag->Read(L"FriendlyName", &var, 0);
                    
                    if (SUCCEEDED(hr)) {
                        CameraInfo info;
                        info.name = var.bstrVal;
                        info.isAvailable = true;
                        info.isIntercepted = false;
                        
                        // Get device path
                        VariantClear(&var);
                        hr = pPropBag->Read(L"DevicePath", &var, 0);
                        if (SUCCEEDED(hr)) {
                            info.devicePath = var.bstrVal;
                        }
                        
                        cameras.push_back(info);
                        VariantClear(&var);
                    }
                    pPropBag->Release();
                }
                pMoniker->Release();
            }
            pEnum->Release();
        }
        pDevEnum->Release();
    }
    
    CoUninitialize();
    return cameras;
}

bool SimpleCameraManager::TestCameraAccess(const std::wstring& devicePath)
{
    std::wcout << L"[SimpleCameraManager] Testing access to: " << devicePath << std::endl;
    
    CameraInterceptor interceptor;
    bool success = interceptor.InterceptCamera(-1);  // Try first available
    
    if (success) {
        std::wcout << L"[SimpleCameraManager] ✅ Camera access successful" << std::endl;
        interceptor.ReleaseCamera();
    } else {
        std::wcout << L"[SimpleCameraManager] ❌ Camera access failed" << std::endl;
    }
    
    return success;
}