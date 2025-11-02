#include "virtual_camera_registry.h"
#include <iostream>
#include <dshow.h>
#include <strmif.h>
#include <comdef.h>

bool VirtualCameraRegistry::RegisterVirtualCamera()
{
    std::wcout << L"[Registry] Registering MySubstitute Virtual Camera..." << std::endl;
    
    bool success = true;
    
    // Step 1: Create DirectShow filter entries
    if (!CreateDirectShowEntries()) {
        std::wcout << L"[Registry] Failed to create DirectShow entries" << std::endl;
        success = false;
    }
    
    // Step 2: Create Media Foundation entries
    if (!CreateMediaFoundationEntries()) {
        std::wcout << L"[Registry] Failed to create Media Foundation entries (may not be critical)" << std::endl;
    }
    
    // Step 3: Create device enumeration entries
    if (!CreateDeviceEnumerationEntries()) {
        std::wcout << L"[Registry] Failed to create device enumeration entries" << std::endl;
        success = false;
    }
    
    if (success) {
        std::wcout << L"[Registry] Virtual camera registered successfully!" << std::endl;
        std::wcout << L"[Registry] Restart camera applications to see 'MySubstitute Virtual Camera'" << std::endl;
    } else {
        std::wcout << L"[Registry] Virtual camera registration failed" << std::endl;
    }
    
    return success;
}

bool VirtualCameraRegistry::UnregisterVirtualCamera()
{
    std::wcout << L"[Registry] Unregistering MySubstitute Virtual Camera..." << std::endl;
    
    if (RemoveAllEntries()) {
        std::wcout << L"[Registry] Virtual camera unregistered successfully" << std::endl;
        return true;
    } else {
        std::wcout << L"[Registry] Failed to unregister virtual camera" << std::endl;
        return false;
    }
}

bool VirtualCameraRegistry::IsVirtualCameraRegistered()
{
    HKEY hKey;
    LONG result = RegOpenKeyExW(
        HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\Classes\\CLSID\\{B3F3A1C4-8F9E-4A2D-9B5C-7E6F8D4C9A3B}",
        0,
        KEY_READ,
        &hKey
    );
    
    if (result == ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return true;
    }
    
    return false;
}

void VirtualCameraRegistry::ListAllCameraDevices()
{
    std::wcout << L"[Registry] Enumerating all video capture devices..." << std::endl;
    
    HRESULT hr = CoInitialize(nullptr);
    
    ICreateDevEnum* pDevEnum = nullptr;
    hr = CoCreateInstance(CLSID_SystemDeviceEnum, nullptr, CLSCTX_INPROC_SERVER,
                         IID_ICreateDevEnum, (void**)&pDevEnum);
    
    if (FAILED(hr)) {
        std::wcout << L"[Registry] ❌ Failed to create device enumerator" << std::endl;
        return;
    }
    
    IEnumMoniker* pEnum = nullptr;
    hr = pDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEnum, 0);
    
    if (hr == S_OK) {
        IMoniker* pMoniker = nullptr;
        ULONG cFetched;
        int deviceCount = 0;
        
        while (pEnum->Next(1, &pMoniker, &cFetched) == S_OK) {
            IPropertyBag* pPropBag;
            hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void**)&pPropBag);
            
            if (SUCCEEDED(hr)) {
                VARIANT var;
                VariantInit(&var);
                hr = pPropBag->Read(L"FriendlyName", &var, 0);
                
                if (SUCCEEDED(hr)) {
                    deviceCount++;
                    std::wcout << L"[Registry] 📹 Device " << deviceCount << L": " << var.bstrVal << std::endl;
                    
                    // Check if it's our device
                    if (wcscmp(var.bstrVal, L"MySubstitute Virtual Camera") == 0) {
                        std::wcout << L"[Registry] 🎯 Found our virtual camera!" << std::endl;
                    }
                    
                    VariantClear(&var);
                }
                pPropBag->Release();
            }
            pMoniker->Release();
        }
        
        if (deviceCount == 0) {
            std::wcout << L"[Registry] ℹ️ No video capture devices found" << std::endl;
        } else {
            std::wcout << L"[Registry] 📊 Total devices found: " << deviceCount << std::endl;
        }
    }
    
    if (pEnum) pEnum->Release();
    if (pDevEnum) pDevEnum->Release();
    CoUninitialize();
}

bool VirtualCameraRegistry::CreateDirectShowEntries()
{
    std::wcout << L"[Registry] Creating DirectShow entries..." << std::endl;
    
    // CLSID registration
    const wchar_t* clsidPath = L"SOFTWARE\\Classes\\CLSID\\{B3F3A1C4-8F9E-4A2D-9B5C-7E6F8D4C9A3B}";
    
    if (!CreateRegistryKey(HKEY_LOCAL_MACHINE, clsidPath, nullptr, L"MySubstitute Virtual Camera")) {
        return false;
    }
    
    // InprocServer32 registration
    wchar_t modulePath[MAX_PATH];
    GetModuleFileNameW(GetModuleHandle(nullptr), modulePath, MAX_PATH);
    
    wchar_t inprocPath[512];
    swprintf_s(inprocPath, L"%s\\InprocServer32", clsidPath);
    
    if (!CreateRegistryKey(HKEY_LOCAL_MACHINE, inprocPath, nullptr, modulePath)) {
        return false;
    }
    
    if (!CreateRegistryKey(HKEY_LOCAL_MACHINE, inprocPath, L"ThreadingModel", L"Both")) {
        return false;
    }
    
    std::wcout << L"[Registry] ✅ DirectShow entries created" << std::endl;
    return true;
}

bool VirtualCameraRegistry::CreateMediaFoundationEntries()
{
    std::wcout << L"[Registry] Creating Media Foundation entries..." << std::endl;
    
    // Media Foundation Transform registration
    const wchar_t* mftPath = L"SOFTWARE\\Classes\\MediaFoundation\\Transforms\\Categories\\{DA1E0AFE-07CE-4B57-A658-421C7C5A9C1E}\\{B3F3A1C4-8F9E-4A2D-9B5C-7E6F8D4C9A3B}";
    
    if (CreateRegistryKey(HKEY_LOCAL_MACHINE, mftPath, L"MFTName", L"MySubstitute Virtual Camera")) {
        std::wcout << L"[Registry] ✅ Media Foundation entries created" << std::endl;
        return true;
    }
    
    return false;
}

bool VirtualCameraRegistry::CreateDeviceEnumerationEntries()
{
    std::wcout << L"[Registry] Creating device enumeration entries..." << std::endl;
    
    // Video Input Device Category registration
    const wchar_t* categoryPath = L"SOFTWARE\\Classes\\CLSID\\{860BB310-5D01-11D0-BD3B-00A0C911CE86}\\Instance\\{B3F3A1C4-8F9E-4A2D-9B5C-7E6F8D4C9A3B}";
    
    if (!CreateRegistryKey(HKEY_LOCAL_MACHINE, categoryPath, L"CLSID", L"{B3F3A1C4-8F9E-4A2D-9B5C-7E6F8D4C9A3B}")) {
        return false;
    }
    
    if (!CreateRegistryKey(HKEY_LOCAL_MACHINE, categoryPath, L"FriendlyName", L"MySubstitute Virtual Camera")) {
        return false;
    }
    
    // Also register in the current user registry for better compatibility
    const wchar_t* userCategoryPath = L"SOFTWARE\\Classes\\CLSID\\{860BB310-5D01-11D0-BD3B-00A0C911CE86}\\Instance\\{B3F3A1C4-8F9E-4A2D-9B5C-7E6F8D4C9A3B}";
    
    CreateRegistryKey(HKEY_CURRENT_USER, userCategoryPath, L"CLSID", L"{B3F3A1C4-8F9E-4A2D-9B5C-7E6F8D4C9A3B}");
    CreateRegistryKey(HKEY_CURRENT_USER, userCategoryPath, L"FriendlyName", L"MySubstitute Virtual Camera");
    
    std::wcout << L"[Registry] ✅ Device enumeration entries created" << std::endl;
    return true;
}

bool VirtualCameraRegistry::RemoveAllEntries()
{
    std::wcout << L"[Registry] Removing all registry entries..." << std::endl;
    
    // Remove CLSID entries
    DeleteRegistryKey(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Classes\\CLSID\\{B3F3A1C4-8F9E-4A2D-9B5C-7E6F8D4C9A3B}");
    DeleteRegistryKey(HKEY_CURRENT_USER, L"SOFTWARE\\Classes\\CLSID\\{B3F3A1C4-8F9E-4A2D-9B5C-7E6F8D4C9A3B}");
    
    // Remove category entries
    DeleteRegistryKey(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Classes\\CLSID\\{860BB310-5D01-11D0-BD3B-00A0C911CE86}\\Instance\\{B3F3A1C4-8F9E-4A2D-9B5C-7E6F8D4C9A3B}");
    DeleteRegistryKey(HKEY_CURRENT_USER, L"SOFTWARE\\Classes\\CLSID\\{860BB310-5D01-11D0-BD3B-00A0C911CE86}\\Instance\\{B3F3A1C4-8F9E-4A2D-9B5C-7E6F8D4C9A3B}");
    
    // Remove Media Foundation entries
    DeleteRegistryKey(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Classes\\MediaFoundation\\Transforms\\Categories\\{DA1E0AFE-07CE-4B57-A658-421C7C5A9C1E}\\{B3F3A1C4-8F9E-4A2D-9B5C-7E6F8D4C9A3B}");
    
    std::wcout << L"[Registry] ✅ All entries removed" << std::endl;
    return true;
}

bool VirtualCameraRegistry::CreateRegistryKey(HKEY hRoot, const wchar_t* path, const wchar_t* name, const wchar_t* value)
{
    HKEY hKey;
    DWORD dwDisposition;
    
    LONG result = RegCreateKeyExW(
        hRoot,
        path,
        0,
        nullptr,
        REG_OPTION_NON_VOLATILE,
        KEY_WRITE,
        nullptr,
        &hKey,
        &dwDisposition
    );
    
    if (result != ERROR_SUCCESS) {
        std::wcout << L"[Registry] ❌ Failed to create key: " << path << L" (Error: " << result << L")" << std::endl;
        return false;
    }
    
    if (name != nullptr && value != nullptr) {
        result = RegSetValueExW(
            hKey,
            name,
            0,
            REG_SZ,
            (const BYTE*)value,
            (wcslen(value) + 1) * sizeof(wchar_t)
        );
        
        if (result != ERROR_SUCCESS) {
            std::wcout << L"[Registry] ❌ Failed to set value: " << name << L" (Error: " << result << L")" << std::endl;
            RegCloseKey(hKey);
            return false;
        }
    }
    
    RegCloseKey(hKey);
    return true;
}

bool VirtualCameraRegistry::DeleteRegistryKey(HKEY hRoot, const wchar_t* path)
{
    LONG result = RegDeleteTreeW(hRoot, path);
    
    // ERROR_FILE_NOT_FOUND is OK (key doesn't exist)
    if (result != ERROR_SUCCESS && result != ERROR_FILE_NOT_FOUND) {
        std::wcout << L"[Registry] ⚠️ Failed to delete key: " << path << L" (Error: " << result << L")" << std::endl;
        return false;
    }
    
    return true;
}