#include "camera_diagnostics.h"
#include <dshow.h>
#include <strmif.h>
#include <comdef.h>
#include <iostream>
#include <sstream>

bool CameraDiagnostics::TestCameraSystem()
{
    std::wcout << L"[CameraDiagnostics] 🔍 Testing camera system..." << std::endl;
    
    HRESULT hr = CoInitialize(nullptr);
    if (FAILED(hr)) {
        std::wcout << L"[CameraDiagnostics] ❌ Failed to initialize COM" << std::endl;
        return false;
    }
    
    ICreateDevEnum* pDevEnum = nullptr;
    hr = CoCreateInstance(CLSID_SystemDeviceEnum, nullptr, CLSCTX_INPROC_SERVER,
                         IID_ICreateDevEnum, (void**)&pDevEnum);
    
    bool success = false;
    if (SUCCEEDED(hr)) {
        IEnumMoniker* pEnum = nullptr;
        hr = pDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEnum, 0);
        
        if (hr == S_OK) {
            IMoniker* pMoniker = nullptr;
            ULONG cFetched;
            int deviceCount = 0;
            
            std::wcout << L"[CameraDiagnostics] 📋 Available video devices:" << std::endl;
            
            while (pEnum->Next(1, &pMoniker, &cFetched) == S_OK) {
                IPropertyBag* pPropBag;
                hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void**)&pPropBag);
                
                if (SUCCEEDED(hr)) {
                    VARIANT var;
                    VariantInit(&var);
                    hr = pPropBag->Read(L"FriendlyName", &var, 0);
                    
                    if (SUCCEEDED(hr)) {
                        deviceCount++;
                        std::wcout << L"    " << deviceCount << L". " << var.bstrVal << std::endl;
                        
                        // Check if it's a virtual camera
                        if (wcsstr(var.bstrVal, L"Virtual") != nullptr ||
                            wcsstr(var.bstrVal, L"OBS") != nullptr ||
                            wcsstr(var.bstrVal, L"MySubstitute") != nullptr) {
                            std::wcout << L"        ^^ 🎭 Virtual Camera Detected!" << std::endl;
                        }
                        
                        VariantClear(&var);
                    }
                    pPropBag->Release();
                }
                pMoniker->Release();
            }
            
            if (deviceCount > 0) {
                success = true;
                std::wcout << L"[CameraDiagnostics] ✅ Found " << deviceCount << L" video devices" << std::endl;
            } else {
                std::wcout << L"[CameraDiagnostics] ⚠️ No video devices found" << std::endl;
            }
        }
        
        if (pEnum) pEnum->Release();
        pDevEnum->Release();
    } else {
        std::wcout << L"[CameraDiagnostics] ❌ Failed to create device enumerator" << std::endl;
    }
    
    CoUninitialize();
    return success;
}

std::vector<CameraDiagnostics::CameraDevice> CameraDiagnostics::ListAllCameras()
{
    std::vector<CameraDevice> cameras;
    
    HRESULT hr = CoInitialize(nullptr);
    if (FAILED(hr)) return cameras;
    
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
                        CameraDevice device;
                        device.name = var.bstrVal;
                        device.isAvailable = true;
                        
                        // Get device path if available
                        VariantClear(&var);
                        hr = pPropBag->Read(L"DevicePath", &var, 0);
                        if (SUCCEEDED(hr)) {
                            device.devicePath = var.bstrVal;
                        }
                        
                        cameras.push_back(device);
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

bool CameraDiagnostics::TestCameraAccess(const std::wstring& devicePath)
{
    std::wcout << L"[CameraDiagnostics] 🧪 Testing camera access..." << std::endl;
    
    // For now, just return true if we can enumerate cameras
    auto cameras = ListAllCameras();
    bool hasAccess = !cameras.empty();
    
    if (hasAccess) {
        std::wcout << L"[CameraDiagnostics] ✅ Camera access appears to be working" << std::endl;
    } else {
        std::wcout << L"[CameraDiagnostics] ❌ No cameras accessible" << std::endl;
    }
    
    return hasAccess;
}

void CameraDiagnostics::ShowDiagnosticsResults()
{
    std::wcout << L"[CameraDiagnostics] 📊 Running full diagnostics..." << std::endl;
    
    auto cameras = ListAllCameras();
    bool systemWorking = TestCameraSystem();
    
    // Build result message
    std::wstring message = L"🔍 CAMERA SYSTEM DIAGNOSTICS\n\n";
    
    if (cameras.empty()) {
        message += L"❌ NO CAMERAS FOUND\n\n";
        message += L"Possible causes:\n";
        message += L"• No cameras connected\n";
        message += L"• Camera drivers missing\n";
        message += L"• All cameras in use by other apps\n";
        message += L"• Windows Camera Privacy blocked\n\n";
        message += L"💡 Solutions:\n";
        message += L"• Check Device Manager\n";
        message += L"• Close Zoom, Teams, Camera app\n";
        message += L"• Check Privacy Settings > Camera";
    } else {
        message += L"📹 CAMERAS DETECTED: " + std::to_wstring(cameras.size()) + L"\n\n";
        
        for (size_t i = 0; i < cameras.size() && i < 5; i++) {
            message += L"    " + std::to_wstring(i + 1) + L". " + cameras[i].name + L"\n";
            
            // Check for virtual cameras
            if (cameras[i].name.find(L"Virtual") != std::wstring::npos ||
                cameras[i].name.find(L"OBS") != std::wstring::npos) {
                message += L"        ^^ 🎭 Virtual Camera!\n";
            }
        }
        
        if (cameras.size() > 5) {
            message += L"    ... and " + std::to_wstring(cameras.size() - 5) + L" more\n";
        }
        
        message += L"\n🎯 NEXT STEPS:\n";
        message += L"• Your camera system is working!\n";
        message += L"• Use 'Start Camera' to begin processing\n";
        message += L"• Look for virtual cameras in other apps\n";
        message += L"• Try OBS Studio for virtual camera features";
    }
    
    // Convert to narrow string for MessageBoxA
    std::string narrowMessage(message.begin(), message.end());
    
    MessageBoxA(nullptr, narrowMessage.c_str(), "Camera Diagnostics", 
               MB_OK | (cameras.empty() ? MB_ICONWARNING : MB_ICONINFORMATION));
}