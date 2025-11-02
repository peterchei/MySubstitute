#include "media_foundation_camera.h"
#include <iostream>
#include <comdef.h>

#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")

HRESULT MediaFoundationVirtualCamera::RegisterVirtualCamera()
{
    std::wcout << L"🔄 Registering MediaFoundation Virtual Camera..." << std::endl;
    
    HRESULT hr = MFStartup(MF_VERSION);
    if (FAILED(hr)) {
        std::wcout << L"❌ Failed to initialize Media Foundation: 0x" << std::hex << hr << std::endl;
        return hr;
    }

    hr = CreateRegistryEntries();
    if (SUCCEEDED(hr)) {
        std::wcout << L"✅ Virtual camera registered successfully!" << std::endl;
        ShowStatus();
    } else {
        std::wcout << L"❌ Failed to create registry entries: 0x" << std::hex << hr << std::endl;
    }

    MFShutdown();
    return hr;
}

HRESULT MediaFoundationVirtualCamera::UnregisterVirtualCamera()
{
    std::wcout << L"🗑️ Unregistering Virtual Camera..." << std::endl;
    
    HRESULT hr = DeleteRegistryEntries();
    if (SUCCEEDED(hr)) {
        std::wcout << L"✅ Virtual camera unregistered successfully!" << std::endl;
    } else {
        std::wcout << L"❌ Failed to delete registry entries: 0x" << std::hex << hr << std::endl;
    }
    
    return hr;
}

HRESULT MediaFoundationVirtualCamera::CreateRegistryEntries()
{
    HKEY hKey;
    LONG result;
    
    // Create device registry key under SYSTEM\CurrentControlSet\Control\DeviceClasses\{65e8773d-8f56-11d0-a3b9-00a0c9223196}
    const wchar_t* deviceClassKey = L"SYSTEM\\CurrentControlSet\\Control\\DeviceClasses\\{65e8773d-8f56-11d0-a3b9-00a0c9223196}\\##?#ROOT#CAMERA#MySubstitute#{65e8773d-8f56-11d0-a3b9-00a0c9223196}";
    
    result = RegCreateKeyExW(
        HKEY_LOCAL_MACHINE,
        deviceClassKey,
        0,
        NULL,
        REG_OPTION_NON_VOLATILE,
        KEY_WRITE,
        NULL,
        &hKey,
        NULL
    );
    
    if (result == ERROR_SUCCESS) {
        // Set device interface properties
        const wchar_t* friendlyName = GetCameraName();
        RegSetValueExW(hKey, L"FriendlyName", 0, REG_SZ, 
                      (const BYTE*)friendlyName, 
                      (DWORD)((wcslen(friendlyName) + 1) * sizeof(wchar_t)));
        
        const wchar_t* description = GetCameraDescription();
        RegSetValueExW(hKey, L"DeviceDesc", 0, REG_SZ,
                      (const BYTE*)description,
                      (DWORD)((wcslen(description) + 1) * sizeof(wchar_t)));
        
        DWORD deviceType = 2; // Camera device type
        RegSetValueExW(hKey, L"DeviceType", 0, REG_DWORD,
                      (const BYTE*)&deviceType, sizeof(DWORD));
        
        RegCloseKey(hKey);
    }
    
    // Also create entry under Media Foundation device source
    const wchar_t* mfDeviceKey = L"SOFTWARE\\Microsoft\\Windows Media Foundation\\Platform\\Default\\MediaSources\\{A3FCE0F0-3824-4902-B1D1-3930B4F3A5B7}";
    
    result = RegCreateKeyExW(
        HKEY_LOCAL_MACHINE,
        mfDeviceKey,
        0,
        NULL,
        REG_OPTION_NON_VOLATILE,
        KEY_WRITE,
        NULL,
        &hKey,
        NULL
    );
    
    if (result == ERROR_SUCCESS) {
        const wchar_t* friendlyName = GetCameraName();
        RegSetValueExW(hKey, L"FriendlyName", 0, REG_SZ,
                      (const BYTE*)friendlyName,
                      (DWORD)((wcslen(friendlyName) + 1) * sizeof(wchar_t)));
        
        // Set source type as video capture
        GUID sourceType = MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID;
        RegSetValueExW(hKey, L"SourceType", 0, REG_BINARY,
                      (const BYTE*)&sourceType, sizeof(GUID));
        
        RegCloseKey(hKey);
    }
    
    return (result == ERROR_SUCCESS) ? S_OK : HRESULT_FROM_WIN32(result);
}

HRESULT MediaFoundationVirtualCamera::DeleteRegistryEntries()
{
    LONG result1 = RegDeleteTreeW(HKEY_LOCAL_MACHINE, 
        L"SYSTEM\\CurrentControlSet\\Control\\DeviceClasses\\{65e8773d-8f56-11d0-a3b9-00a0c9223196}\\##?#ROOT#CAMERA#MySubstitute#{65e8773d-8f56-11d0-a3b9-00a0c9223196}");
    
    LONG result2 = RegDeleteTreeW(HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\Microsoft\\Windows Media Foundation\\Platform\\Default\\MediaSources\\{A3FCE0F0-3824-4902-B1D1-3930B4F3A5B7}");
    
    return ((result1 == ERROR_SUCCESS || result1 == ERROR_FILE_NOT_FOUND) &&
            (result2 == ERROR_SUCCESS || result2 == ERROR_FILE_NOT_FOUND)) ? S_OK : E_FAIL;
}

HRESULT MediaFoundationVirtualCamera::TestCameraVisibility()
{
    std::wcout << L"🔍 Testing camera visibility..." << std::endl;
    
    HRESULT hr = MFStartup(MF_VERSION);
    if (FAILED(hr)) {
        std::wcout << L"❌ Failed to initialize MF: 0x" << std::hex << hr << std::endl;
        return hr;
    }
    
    hr = EnumerateVideoDevices();
    
    MFShutdown();
    return hr;
}

HRESULT MediaFoundationVirtualCamera::EnumerateVideoDevices()
{
    IMFAttributes* pAttributes = nullptr;
    IMFActivate** ppDevices = nullptr;
    UINT32 count = 0;
    
    HRESULT hr = MFCreateAttributes(&pAttributes, 1);
    if (FAILED(hr)) {
        std::wcout << L"❌ Failed to create attributes" << std::endl;
        return hr;
    }
    
    hr = pAttributes->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);
    if (FAILED(hr)) {
        pAttributes->Release();
        return hr;
    }
    
    hr = MFEnumDeviceSources(pAttributes, &ppDevices, &count);
    if (FAILED(hr)) {
        std::wcout << L"❌ Failed to enumerate devices: 0x" << std::hex << hr << std::endl;
        pAttributes->Release();
        return hr;
    }
    
    std::wcout << L"📹 Found " << count << L" video devices:" << std::endl;
    
    bool foundOurCamera = false;
    for (UINT32 i = 0; i < count; i++) {
        WCHAR* szFriendlyName = nullptr;
        UINT32 cchName = 0;
        
        hr = ppDevices[i]->GetAllocatedString(MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME, &szFriendlyName, &cchName);
        if (SUCCEEDED(hr)) {
            std::wcout << L"  [" << i << L"] " << szFriendlyName << std::endl;
            
            if (wcsstr(szFriendlyName, L"MySubstitute") != nullptr) {
                foundOurCamera = true;
                std::wcout << L"      ✅ Found our virtual camera!" << std::endl;
            }
            
            CoTaskMemFree(szFriendlyName);
        }
        
        ppDevices[i]->Release();
    }
    
    if (!foundOurCamera) {
        std::wcout << L"❌ MySubstitute Virtual Camera not found in device list" << std::endl;
    }
    
    CoTaskMemFree(ppDevices);
    pAttributes->Release();
    
    return S_OK;
}

bool MediaFoundationVirtualCamera::IsRegistered()
{
    HKEY hKey;
    LONG result = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\Microsoft\\Windows Media Foundation\\Platform\\Default\\MediaSources\\{A3FCE0F0-3824-4902-B1D1-3930B4F3A5B7}",
        0, KEY_READ, &hKey);
    
    if (result == ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return true;
    }
    return false;
}

void MediaFoundationVirtualCamera::ShowStatus()
{
    std::wcout << L"\n📊 Virtual Camera Status:" << std::endl;
    std::wcout << L"    Registered: " << (IsRegistered() ? L"✅" : L"❌") << std::endl;
    
    std::wcout << L"\n🧪 Testing device visibility..." << std::endl;
    TestCameraVisibility();
    
    std::wcout << L"\n📋 Next steps:" << std::endl;
    std::wcout << L"    1. Open Camera app (Win+S, search 'Camera')" << std::endl;
    std::wcout << L"    2. Look for 'MySubstitute Virtual Camera' in device list" << std::endl;
    std::wcout << L"    3. Try switching to it in camera settings" << std::endl;
}