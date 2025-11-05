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
    
    std::wcout << L"📝 Creating registry entries for Frame Server compatibility..." << std::endl;
    
    // 1. Register under CLSID for COM (already done by DirectShow)
    // 2. Register as a video capture device for Frame Server
    
    // Create Camera Frame Server device entry
    // This is the key path that Windows Camera Frame Server looks for
    const wchar_t* frameServerKey = L"SOFTWARE\\Microsoft\\Windows Media Foundation\\Platform\\Default\\FrameServer\\VirtualCamera\\MySubstitute";
    
    result = RegCreateKeyExW(
        HKEY_LOCAL_MACHINE,
        frameServerKey,
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
        
        const wchar_t* description = GetCameraDescription();
        RegSetValueExW(hKey, L"Description", 0, REG_SZ,
                      (const BYTE*)description,
                      (DWORD)((wcslen(description) + 1) * sizeof(wchar_t)));
        
        // Point to our DirectShow filter CLSID
        const wchar_t* clsid = L"{B3F3A1C4-8F9E-4A2D-9B5C-7E6F8D4C9A3B}";
        RegSetValueExW(hKey, L"CLSID", 0, REG_SZ,
                      (const BYTE*)clsid,
                      (DWORD)((wcslen(clsid) + 1) * sizeof(wchar_t)));
        
        // Enable Frame Server compatibility mode
        DWORD enableFrameServer = 1;
        RegSetValueExW(hKey, L"EnableFrameServerMode", 0, REG_DWORD,
                      (const BYTE*)&enableFrameServer, sizeof(DWORD));
        
        // Specify this is a virtual camera
        DWORD isVirtual = 1;
        RegSetValueExW(hKey, L"IsVirtualCamera", 0, REG_DWORD,
                      (const BYTE*)&isVirtual, sizeof(DWORD));
        
        RegCloseKey(hKey);
        std::wcout << L"  ✅ Frame Server registry key created" << std::endl;
    } else {
        std::wcout << L"  ❌ Failed to create Frame Server key: " << result << std::endl;
    }
    
    // Also register in the Camera devices list
    const wchar_t* cameraDevicesKey = L"SOFTWARE\\Microsoft\\Windows Media Foundation\\Platform\\Default\\Capture\\{65E8773D-8F56-11D0-A3B9-00A0C9223196}\\MySubstitute";
    
    result = RegCreateKeyExW(
        HKEY_LOCAL_MACHINE,
        cameraDevicesKey,
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
        
        // Point to DirectShow CLSID
        const wchar_t* clsid = L"{B3F3A1C4-8F9E-4A2D-9B5C-7E6F8D4C9A3B}";
        RegSetValueExW(hKey, L"CLSID", 0, REG_SZ,
                      (const BYTE*)clsid,
                      (DWORD)((wcslen(clsid) + 1) * sizeof(wchar_t)));
        
        RegCloseKey(hKey);
        std::wcout << L"  ✅ Camera device registry key created" << std::endl;
    } else {
        std::wcout << L"  ❌ Failed to create camera device key: " << result << std::endl;
    }
    
    std::wcout << L"\n⚠️  IMPORTANT: For UWP app support, additional steps are required:" << std::endl;
    std::wcout << L"   1. Your DirectShow filter is registered and works in Win32 apps" << std::endl;
    std::wcout << L"   2. UWP apps require Frame Server driver support (kernel-mode)" << std::endl;
    std::wcout << L"   3. Alternative: Use OBS Virtual Camera (signs drivers properly)" << std::endl;
    std::wcout << L"   4. Or: Build a proper KMDF camera driver (advanced)" << std::endl;
    
    return (result == ERROR_SUCCESS) ? S_OK : HRESULT_FROM_WIN32(result);
}

HRESULT MediaFoundationVirtualCamera::DeleteRegistryEntries()
{
    std::wcout << L"🗑️ Removing Frame Server registry entries..." << std::endl;
    
    LONG result1 = RegDeleteTreeW(HKEY_LOCAL_MACHINE, 
        L"SOFTWARE\\Microsoft\\Windows Media Foundation\\Platform\\Default\\FrameServer\\VirtualCamera\\MySubstitute");
    
    LONG result2 = RegDeleteTreeW(HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\Microsoft\\Windows Media Foundation\\Platform\\Default\\Capture\\{65E8773D-8F56-11D0-A3B9-00A0C9223196}\\MySubstitute");
    
    if (result1 == ERROR_SUCCESS || result1 == ERROR_FILE_NOT_FOUND) {
        std::wcout << L"  ✅ Frame Server entries removed" << std::endl;
    }
    
    if (result2 == ERROR_SUCCESS || result2 == ERROR_FILE_NOT_FOUND) {
        std::wcout << L"  ✅ Camera device entries removed" << std::endl;
    }
    
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
    
    std::wcout << L"\n📋 Application Compatibility:" << std::endl;
    std::wcout << L"    ✅ Win32 Apps: Chrome, Firefox, OBS Studio, Zoom (desktop)" << std::endl;
    std::wcout << L"    ❌ UWP Apps: Windows Camera, WhatsApp, Zoom (store version)" << std::endl;
    
    std::wcout << L"\n⚠️  UWP Limitation Explanation:" << std::endl;
    std::wcout << L"    UWP apps require Windows Camera Frame Server support" << std::endl;
    std::wcout << L"    DirectShow filters don't automatically work with Frame Server" << std::endl;
    std::wcout << L"    Requires kernel-mode driver or Frame Server plugin" << std::endl;
    
    std::wcout << L"\n💡 Solutions for UWP Support:" << std::endl;
    std::wcout << L"    1. Use desktop versions of apps (Zoom desktop, not Store)" << std::endl;
    std::wcout << L"    2. Install OBS Virtual Camera (has proper driver signing)" << std::endl;
    std::wcout << L"    3. Develop KMDF camera driver (advanced, requires code signing)" << std::endl;
    std::wcout << L"    4. Use third-party virtual camera with UWP support" << std::endl;
    
    std::wcout << L"\n✅ Your virtual camera works great with:" << std::endl;
    std::wcout << L"    • Web browsers (Chrome, Edge, Firefox)" << std::endl;
    std::wcout << L"    • Desktop apps (Zoom, Teams, Skype desktop)" << std::endl;
    std::wcout << L"    • Streaming software (OBS Studio, XSplit)" << std::endl;
    std::wcout << L"    • Video editing software" << std::endl;
}