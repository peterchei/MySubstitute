#include "simple_registry_virtual_camera.h"
#include <iostream>
#include <dshow.h>
#include <comdef.h>
#include <shellapi.h>
#include <sddl.h>
#include <shlwapi.h>

#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "advapi32.lib")

const wchar_t* SimpleRegistryVirtualCamera::GetDeviceGUID()
{
    return L"{B3F3A1C4-8F9E-4A2D-9B5C-7E6F8D4C9A3B}";
}

const wchar_t* SimpleRegistryVirtualCamera::GetDeviceName()
{
    return L"MySubstitute Virtual Camera";
}

const wchar_t* SimpleRegistryVirtualCamera::GetDevicePath()
{
    return L"\\\\?\\MySubstitute#VirtualCamera#1";
}

bool SimpleRegistryVirtualCamera::CheckIfUserIsAdmin()
{
    BOOL isAdmin = FALSE;
    PSID adminGroup = nullptr;
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;

    if (AllocateAndInitializeSid(&ntAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &adminGroup)) {
        CheckTokenMembership(nullptr, adminGroup, &isAdmin);
        FreeSid(adminGroup);
    }
    
    return isAdmin != FALSE;
}

bool SimpleRegistryVirtualCamera::CreateVirtualCameraDevice()
{
    std::wcout << L"[SimpleRegistry] 📝 Creating virtual camera device..." << std::endl;
    
    return CreateDeviceRegistryEntries() && 
           CreateDirectShowEntries() && 
           CreateDeviceInterfaceEntries();
}

bool SimpleRegistryVirtualCamera::RemoveVirtualCameraDevice()
{
    std::wcout << L"[SimpleRegistry] 🗑️ Removing virtual camera device..." << std::endl;
    
    // Remove DirectShow entries
    std::wstring clsidPath = L"SOFTWARE\\Classes\\CLSID\\";
    clsidPath += GetDeviceGUID();
    RegDeleteTreeW(HKEY_LOCAL_MACHINE, clsidPath.c_str());
    RegDeleteTreeW(HKEY_CURRENT_USER, clsidPath.c_str());
    
    // Remove from video input device category
    std::wstring categoryPath = L"SOFTWARE\\Classes\\CLSID\\{860BB310-5D01-11D0-BD3B-00A0C911CE86}\\Instance\\";
    categoryPath += GetDeviceGUID();
    RegDeleteTreeW(HKEY_LOCAL_MACHINE, categoryPath.c_str());
    RegDeleteTreeW(HKEY_CURRENT_USER, categoryPath.c_str());
    
    return true;
}

bool SimpleRegistryVirtualCamera::IsVirtualCameraRegistered()
{
    HKEY hKey;
    std::wstring path = L"SOFTWARE\\Classes\\CLSID\\";
    path += GetDeviceGUID();
    
    // Check both HKLM and HKCU
    LONG result1 = RegOpenKeyExW(HKEY_LOCAL_MACHINE, path.c_str(), 0, KEY_READ, &hKey);
    if (result1 == ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return true;
    }
    
    LONG result2 = RegOpenKeyExW(HKEY_CURRENT_USER, path.c_str(), 0, KEY_READ, &hKey);
    if (result2 == ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return true;
    }
    
    return false;
}

bool SimpleRegistryVirtualCamera::TestDeviceVisibility()
{
    std::wcout << L"[SimpleRegistry] 🔍 Testing device visibility..." << std::endl;
    
    HRESULT hr = CoInitialize(nullptr);
    if (FAILED(hr)) return false;
    
    ICreateDevEnum* pDevEnum = nullptr;
    hr = CoCreateInstance(CLSID_SystemDeviceEnum, nullptr, CLSCTX_INPROC_SERVER,
                         IID_ICreateDevEnum, (void**)&pDevEnum);
    
    bool found = false;
    if (SUCCEEDED(hr)) {
        IEnumMoniker* pEnum = nullptr;
        hr = pDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEnum, 0);
        
        if (hr == S_OK) {
            IMoniker* pMoniker = nullptr;
            ULONG cFetched;
            int deviceCount = 0;
            
            std::wcout << L"[SimpleRegistry] 📋 Scanning video input devices:" << std::endl;
            
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
                        
                        if (wcsstr(var.bstrVal, L"MySubstitute") != nullptr) {
                            found = true;
                            std::wcout << L"[SimpleRegistry] 🎯 FOUND MySubstitute Virtual Camera!" << std::endl;
                        }
                        
                        VariantClear(&var);
                    }
                    pPropBag->Release();
                }
                pMoniker->Release();
            }
            
            std::wcout << L"[SimpleRegistry] 📊 Total devices found: " << deviceCount << std::endl;
            
            pEnum->Release();
        } else {
            std::wcout << L"[SimpleRegistry] ⚠️ No video input devices enumerated" << std::endl;
        }
        pDevEnum->Release();
    }
    
    CoUninitialize();
    return found;
}

void SimpleRegistryVirtualCamera::ShowDetailedStatus()
{
    bool isRegistered = IsVirtualCameraRegistered();
    bool isVisible = TestDeviceVisibility();
    bool isAdmin = CheckIfUserIsAdmin();
    
    std::wcout << L"[SimpleRegistry] 📊 Detailed Status Report:" << std::endl;
    std::wcout << L"    Administrator: " << (isAdmin ? L"✅" : L"❌") << std::endl;
    std::wcout << L"    Registered: " << (isRegistered ? L"✅" : L"❌") << std::endl;
    std::wcout << L"    Visible: " << (isVisible ? L"✅" : L"❌") << std::endl;
    
    std::wstring message = L"🔍 VIRTUAL CAMERA DETAILED STATUS\n\n";
    
    // Admin status
    if (isAdmin) {
        message += L"🔐 Administrator: ✅ Running with admin privileges\n";
    } else {
        message += L"🔐 Administrator: ❌ NOT running as admin\n";
        message += L"   ⚠️ Virtual camera registration requires admin rights\n";
    }
    
    // Registration status  
    if (isRegistered) {
        message += L"📋 Registry: ✅ Virtual camera entries exist\n";
    } else {
        message += L"📋 Registry: ❌ No registry entries found\n";
        message += L"   ⚠️ Need to register virtual camera first\n";
    }
    
    // Visibility status
    if (isVisible) {
        message += L"👁️ Visibility: ✅ Device appears in DirectShow enumeration\n";
        message += L"\n🎉 SUCCESS! Virtual camera is working correctly!\n";
        message += L"• MySubstitute Virtual Camera should appear in:\n";
        message += L"  - Windows Camera app\n";
        message += L"  - Zoom, Teams, Discord, Skype\n";
        message += L"  - Web browsers (Chrome, Edge, Firefox)\n";
        message += L"  - All video applications\n";
    } else {
        message += L"👁️ Visibility: ❌ Device not appearing in applications\n";
        
        if (isRegistered) {
            message += L"\n⚠️ ISSUE: Registered but not visible\n";
            message += L"Possible causes:\n";
            message += L"• Need to restart camera applications\n";
            message += L"• DirectShow cache needs refresh\n";
            message += L"• Missing DirectShow base classes\n";
            message += L"• System security restrictions\n";
        } else {
            message += L"\n❌ ISSUE: Not registered\n";
            message += L"• Run as Administrator\n";
            message += L"• Try registration again\n";
        }
    }
    
    // Recommendations
    message += L"\n💡 RECOMMENDATIONS:\n";
    if (!isAdmin) {
        message += L"1. Right-click MySubstitute.exe → 'Run as administrator'\n";
        message += L"2. Try registration again\n";
    } else if (!isRegistered) {
        message += L"1. Click 'Register Virtual Camera' again\n";
        message += L"2. Check Windows Event Log for errors\n";
    } else if (!isVisible) {
        message += L"1. Restart Camera app, Zoom, Teams, etc.\n";
        message += L"2. Try OBS Studio as reliable alternative\n";
        message += L"3. Check antivirus is not blocking\n";
    } else {
        message += L"1. Virtual camera is working! Test in Camera app\n";
        message += L"2. Use 'Start Virtual Camera' to begin streaming\n";
    }
    
    // Convert to narrow string
    std::string narrowMessage(message.begin(), message.end());
    MessageBoxA(nullptr, narrowMessage.c_str(), "Virtual Camera Status", 
               MB_OK | (isVisible ? MB_ICONINFORMATION : MB_ICONWARNING));
}

bool SimpleRegistryVirtualCamera::RegisterWithAdminCheck()
{
    if (!CheckIfUserIsAdmin()) {
        MessageBoxA(nullptr,
            "🔐 Administrator Privileges Required\n\n"
            "Virtual camera registration requires administrator privileges.\n\n"
            "📋 How to run as administrator:\n"
            "1. Close MySubstitute\n"
            "2. Right-click MySubstitute.exe\n"
            "3. Select 'Run as administrator'\n"
            "4. Try registration again\n\n"
            "⚠️ This is required to modify Windows registry for DirectShow devices.",
            "Admin Required", MB_OK | MB_ICONWARNING);
        return false;
    }
    
    return CreateVirtualCameraDevice();
}

bool SimpleRegistryVirtualCamera::CreateDeviceRegistryEntries()
{
    std::wcout << L"[SimpleRegistry] 📝 Creating device registry entries..." << std::endl;
    
    // Try both HKLM and HKCU (HKCU as fallback if HKLM fails)
    HKEY rootKey = HKEY_LOCAL_MACHINE;
    std::wstring basePath = L"SOFTWARE\\Classes\\CLSID\\";
    basePath += GetDeviceGUID();
    
    HKEY hKey;
    LONG result = RegCreateKeyExW(rootKey, basePath.c_str(), 0, nullptr,
        REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKey, nullptr);
    
    if (result != ERROR_SUCCESS) {
        // Try HKCU as fallback
        std::wcout << L"[SimpleRegistry] ⚠️ HKLM failed, trying HKCU..." << std::endl;
        rootKey = HKEY_CURRENT_USER;
        result = RegCreateKeyExW(rootKey, basePath.c_str(), 0, nullptr,
            REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKey, nullptr);
    }
    
    if (result != ERROR_SUCCESS) {
        std::wcout << L"[SimpleRegistry] ❌ Failed to create CLSID key: " << result << std::endl;
        return false;
    }
    
    // Set device name
    const wchar_t* deviceName = GetDeviceName();
    RegSetValueExW(hKey, nullptr, 0, REG_SZ, (const BYTE*)deviceName,
                   (wcslen(deviceName) + 1) * sizeof(wchar_t));
    
    RegCloseKey(hKey);
    std::wcout << L"[SimpleRegistry] ✅ Device registry entries created" << std::endl;
    return true;
}

bool SimpleRegistryVirtualCamera::CreateDirectShowEntries()
{
    std::wcout << L"[SimpleRegistry] 📺 Creating DirectShow entries..." << std::endl;
    
    // Register in video input device category  
    // {860BB310-5D01-11D0-BD3B-00A0C911CE86} = CLSID_VideoInputDeviceCategory
    std::wstring categoryPath = L"SOFTWARE\\Classes\\CLSID\\{860BB310-5D01-11D0-BD3B-00A0C911CE86}\\Instance\\";
    categoryPath += GetDeviceGUID();
    
    HKEY hKey;
    LONG result = RegCreateKeyExW(HKEY_LOCAL_MACHINE, categoryPath.c_str(), 0, nullptr,
        REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKey, nullptr);
    
    if (result != ERROR_SUCCESS) {
        // Try HKCU
        result = RegCreateKeyExW(HKEY_CURRENT_USER, categoryPath.c_str(), 0, nullptr,
            REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKey, nullptr);
    }
    
    if (result != ERROR_SUCCESS) {
        std::wcout << L"[SimpleRegistry] ❌ Failed to create DirectShow category: " << result << std::endl;
        return false;
    }
    
    // Set CLSID
    const wchar_t* clsid = GetDeviceGUID();
    RegSetValueExW(hKey, L"CLSID", 0, REG_SZ, (const BYTE*)clsid,
                   (wcslen(clsid) + 1) * sizeof(wchar_t));
    
    // Set friendly name
    const wchar_t* friendlyName = GetDeviceName();
    RegSetValueExW(hKey, L"FriendlyName", 0, REG_SZ, (const BYTE*)friendlyName,
                   (wcslen(friendlyName) + 1) * sizeof(wchar_t));
    
    RegCloseKey(hKey);
    std::wcout << L"[SimpleRegistry] ✅ DirectShow entries created" << std::endl;
    return true;
}

bool SimpleRegistryVirtualCamera::CreateDeviceInterfaceEntries()
{
    std::wcout << L"[SimpleRegistry] 🔌 Creating device interface entries..." << std::endl;
    
    // Create additional entries that might help with device detection
    // This creates entries under the video capture device interface
    
    std::wstring interfacePath = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\DeviceAccess\\Global\\{E5323777-F976-4f5b-9B55-B94699C46E44}";
    
    HKEY hKey;
    LONG result = RegCreateKeyExW(HKEY_LOCAL_MACHINE, interfacePath.c_str(), 0, nullptr,
        REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKey, nullptr);
    
    if (result != ERROR_SUCCESS) {
        result = RegCreateKeyExW(HKEY_CURRENT_USER, interfacePath.c_str(), 0, nullptr,
            REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKey, nullptr);
    }
    
    if (result == ERROR_SUCCESS) {
        // Set device path
        const wchar_t* devicePath = GetDevicePath();
        RegSetValueExW(hKey, L"MySubstituteCamera", 0, REG_SZ, (const BYTE*)devicePath,
                       (wcslen(devicePath) + 1) * sizeof(wchar_t));
        RegCloseKey(hKey);
    }
    
    std::wcout << L"[SimpleRegistry] ✅ Device interface entries created" << std::endl;
    return true;
}

bool SimpleRegistryVirtualCamera::VerifyAllEntries()
{
    return IsVirtualCameraRegistered();
}