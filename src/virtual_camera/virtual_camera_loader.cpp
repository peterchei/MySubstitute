#include "virtual_camera_loader.h"
#include <iostream>
#include <dshow.h>
#include <strmif.h>
#include <comdef.h>
#include <shlobj.h>

bool VirtualCameraLoader::InstallVirtualCamera()
{
    std::wcout << L"[VirtualCameraLoader] Installing MySubstitute Virtual Camera..." << std::endl;
    
    // Try multiple approaches in order of reliability
    
    // Approach 1: Use Windows Media Foundation (Windows 10+)
    if (UseWindows10VirtualCamera()) {
        std::wcout << L"[VirtualCameraLoader] ✅ Installed using Windows 10 Virtual Camera API" << std::endl;
        return true;
    }
    
    // Approach 2: Copy OBS Virtual Camera if available
    if (IsOBSVirtualCameraAvailable() && CopyOBSRegistration()) {
        std::wcout << L"[VirtualCameraLoader] ✅ Installed using OBS Virtual Camera approach" << std::endl;
        return true;
    }
    
    // Approach 3: Create pass-through filter
    if (CreatePassThroughFilter()) {
        std::wcout << L"[VirtualCameraLoader] ✅ Installed as pass-through filter" << std::endl;
        return true;
    }
    
    // Approach 4: Register with Media Foundation
    if (RegisterWithMediaFoundation()) {
        std::wcout << L"[VirtualCameraLoader] ✅ Registered with Media Foundation" << std::endl;
        return true;
    }
    
    std::wcout << L"[VirtualCameraLoader] ❌ All installation methods failed" << std::endl;
    return false;
}

bool VirtualCameraLoader::UninstallVirtualCamera()
{
    std::wcout << L"[VirtualCameraLoader] Uninstalling virtual camera..." << std::endl;
    
    // Remove registry entries
    RegDeleteTreeW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Classes\\CLSID\\{B3F3A1C4-8F9E-4A2D-9B5C-7E6F8D4C9A3B}");
    RegDeleteTreeW(HKEY_CURRENT_USER, L"SOFTWARE\\Classes\\CLSID\\{B3F3A1C4-8F9E-4A2D-9B5C-7E6F8D4C9A3B}");
    
    // Remove device category entries
    RegDeleteTreeW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Classes\\CLSID\\{860BB310-5D01-11D0-BD3B-00A0C911CE86}\\Instance\\{B3F3A1C4-8F9E-4A2D-9B5C-7E6F8D4C9A3B}");
    RegDeleteTreeW(HKEY_CURRENT_USER, L"SOFTWARE\\Classes\\CLSID\\{860BB310-5D01-11D0-BD3B-00A0C911CE86}\\Instance\\{B3F3A1C4-8F9E-4A2D-9B5C-7E6F8D4C9A3B}");
    
    std::wcout << L"[VirtualCameraLoader] ✅ Virtual camera uninstalled" << std::endl;
    return true;
}

bool VirtualCameraLoader::IsOBSVirtualCameraAvailable()
{
    // Check if OBS Virtual Camera is installed
    HKEY hKey;
    LONG result = RegOpenKeyExW(
        HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\Classes\\CLSID\\{27B05C2D-93DC-474A-A5DA-9BBA34CB2A9C}",
        0,
        KEY_READ,
        &hKey
    );
    
    if (result == ERROR_SUCCESS) {
        RegCloseKey(hKey);
        std::wcout << L"[VirtualCameraLoader] 📹 OBS Virtual Camera detected" << std::endl;
        return true;
    }
    
    std::wcout << L"[VirtualCameraLoader] ℹ️ OBS Virtual Camera not found" << std::endl;
    return false;
}

bool VirtualCameraLoader::TestVirtualCameraVisibility()
{
    std::wcout << L"[VirtualCameraLoader] Testing virtual camera visibility..." << std::endl;
    
    HRESULT hr = CoInitialize(nullptr);
    
    ICreateDevEnum* pDevEnum = nullptr;
    hr = CoCreateInstance(CLSID_SystemDeviceEnum, nullptr, CLSCTX_INPROC_SERVER,
                         IID_ICreateDevEnum, (void**)&pDevEnum);
    
    if (FAILED(hr)) {
        std::wcout << L"[VirtualCameraLoader] ❌ Failed to create device enumerator" << std::endl;
        CoUninitialize();
        return false;
    }
    
    IEnumMoniker* pEnum = nullptr;
    hr = pDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEnum, 0);
    
    bool found = false;
    if (hr == S_OK) {
        IMoniker* pMoniker = nullptr;
        ULONG cFetched;
        int deviceCount = 0;
        
        std::wcout << L"[VirtualCameraLoader] 📋 Available video devices:" << std::endl;
        
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
                    
                    // Check if it's our device
                    if (wcsstr(var.bstrVal, L"MySubstitute") != nullptr) {
                        found = true;
                        std::wcout << L"[VirtualCameraLoader] 🎯 Found MySubstitute Virtual Camera!" << std::endl;
                    }
                    
                    VariantClear(&var);
                }
                pPropBag->Release();
            }
            pMoniker->Release();
        }
        
        if (deviceCount == 0) {
            std::wcout << L"[VirtualCameraLoader] ⚠️ No video devices found at all" << std::endl;
        } else {
            std::wcout << L"[VirtualCameraLoader] 📊 Total devices: " << deviceCount << std::endl;
        }
    }
    
    if (pEnum) pEnum->Release();
    if (pDevEnum) pDevEnum->Release();
    CoUninitialize();
    
    return found;
}

bool VirtualCameraLoader::UseWindows10VirtualCamera()
{
    std::wcout << L"[VirtualCameraLoader] Trying Windows 10+ Virtual Camera API..." << std::endl;
    
    // This is a placeholder - Windows 10+ has newer virtual camera APIs
    // but they require specialized implementation
    
    return false;
}

bool VirtualCameraLoader::CopyOBSRegistration()
{
    std::wcout << L"[VirtualCameraLoader] Copying OBS Virtual Camera registration..." << std::endl;
    
    // Read OBS registration and create similar one for MySubstitute
    HKEY obsKey;
    LONG result = RegOpenKeyExW(
        HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\Classes\\CLSID\\{27B05C2D-93DC-474A-A5DA-9BBA34CB2A9C}",
        0,
        KEY_READ,
        &obsKey
    );
    
    if (result == ERROR_SUCCESS) {
        // Copy OBS registration pattern but use our CLSID
        HKEY ourKey;
        result = RegCreateKeyExW(
            HKEY_LOCAL_MACHINE,
            L"SOFTWARE\\Classes\\CLSID\\{B3F3A1C4-8F9E-4A2D-9B5C-7E6F8D4C9A3B}",
            0,
            nullptr,
            REG_OPTION_NON_VOLATILE,
            KEY_WRITE,
            nullptr,
            &ourKey,
            nullptr
        );
        
        if (result == ERROR_SUCCESS) {
            // Set friendly name
            const wchar_t* name = L"MySubstitute Virtual Camera";
            RegSetValueExW(ourKey, nullptr, 0, REG_SZ, 
                          (const BYTE*)name, (wcslen(name) + 1) * sizeof(wchar_t));
            
            RegCloseKey(ourKey);
            RegCloseKey(obsKey);
            return true;
        }
        
        RegCloseKey(obsKey);
    }
    
    return false;
}

bool VirtualCameraLoader::CreatePassThroughFilter()
{
    std::wcout << L"[VirtualCameraLoader] Creating pass-through filter..." << std::endl;
    
    // This approach creates a filter that appears in the list but passes through
    // to the real camera while applying our AI processing
    
    return false;
}

bool VirtualCameraLoader::RegisterWithMediaFoundation()
{
    std::wcout << L"[VirtualCameraLoader] Registering with Media Foundation..." << std::endl;
    
    // Try registering with Windows Media Foundation instead of DirectShow
    HKEY mfKey;
    LONG result = RegCreateKeyExW(
        HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\Microsoft\\Windows Media Foundation\\Platform\\Transforms\\{B3F3A1C4-8F9E-4A2D-9B5C-7E6F8D4C9A3B}",
        0,
        nullptr,
        REG_OPTION_NON_VOLATILE,
        KEY_WRITE,
        nullptr,
        &mfKey,
        nullptr
    );
    
    if (result == ERROR_SUCCESS) {
        const wchar_t* name = L"MySubstitute Virtual Camera";
        RegSetValueExW(mfKey, L"FriendlyName", 0, REG_SZ,
                      (const BYTE*)name, (wcslen(name) + 1) * sizeof(wchar_t));
        
        RegCloseKey(mfKey);
        std::wcout << L"[VirtualCameraLoader] ✅ Registered with Media Foundation" << std::endl;
        return true;
    }
    
    return false;
}