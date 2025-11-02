#include "obs_virtual_camera_helper.h"
#include <iostream>
#include <vector>
#include <dshow.h>
#include <comdef.h>
#include <tlhelp32.h>
#include <shellapi.h>

bool OBSVirtualCameraHelper::IsOBSVirtualCameraInstalled()
{
    std::wcout << L"[OBSHelper] 🔍 Checking for OBS Virtual Camera..." << std::endl;
    
    // Method 1: Check registry for OBS Virtual Camera
    if (CheckOBSRegistry()) {
        std::wcout << L"[OBSHelper] ✅ OBS Virtual Camera found in registry" << std::endl;
        return true;
    }
    
    // Method 2: Check DirectShow devices for OBS camera
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
            
            while (pEnum->Next(1, &pMoniker, &cFetched) == S_OK && !found) {
                IPropertyBag* pPropBag;
                hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void**)&pPropBag);
                
                if (SUCCEEDED(hr)) {
                    VARIANT var;
                    VariantInit(&var);
                    hr = pPropBag->Read(L"FriendlyName", &var, 0);
                    
                    if (SUCCEEDED(hr)) {
                        std::wstring name = var.bstrVal;
                        if (name.find(L"OBS Virtual Camera") != std::wstring::npos ||
                            name.find(L"OBS-Camera") != std::wstring::npos) {
                            found = true;
                            std::wcout << L"[OBSHelper] 🎭 Found OBS Virtual Camera: " << name << std::endl;
                        }
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
    
    if (!found) {
        std::wcout << L"[OBSHelper] ❌ OBS Virtual Camera not found" << std::endl;
    }
    
    return found;
}

bool OBSVirtualCameraHelper::IsOBSRunning()
{
    return CheckOBSProcess();
}

void OBSVirtualCameraHelper::ShowOBSInstallationGuide()
{
    std::wcout << L"[OBSHelper] 📋 Showing OBS installation guide..." << std::endl;
    
    int result = MessageBoxA(nullptr,
        "💡 RECOMMENDED SOLUTION: Install OBS Studio\n\n"
        "🎬 OBS Studio (free) includes a proven virtual camera that works\n"
        "with all applications (Zoom, Teams, Discord, browsers).\n\n"
        "✅ Benefits:\n"
        "• Reliable virtual camera infrastructure\n"
        "• Works with all camera applications\n"
        "• MySubstitute can integrate with it\n"
        "• Professional streaming features\n\n"
        "📥 Would you like to download OBS Studio now?\n"
        "(Opens the official OBS website)",
        "Virtual Camera Solution", 
        MB_YESNO | MB_ICONINFORMATION);
    
    if (result == IDYES) {
        // Open OBS website
        ShellExecuteA(nullptr, "open", "https://obsproject.com/", nullptr, nullptr, SW_SHOWNORMAL);
        
        MessageBoxA(nullptr,
            "📋 OBS STUDIO SETUP INSTRUCTIONS\n\n"
            "1️⃣ Download and install OBS Studio from the website\n"
            "2️⃣ Open OBS Studio\n"
            "3️⃣ Click 'Start Virtual Camera' in OBS\n"
            "4️⃣ The virtual camera will appear in all apps\n"
            "5️⃣ MySubstitute can then enhance your camera feed\n\n"
            "💡 After installing OBS, restart MySubstitute to detect it!",
            "Setup Instructions", MB_OK | MB_ICONINFORMATION);
    }
}

bool OBSVirtualCameraHelper::StartOBSVirtualCamera()
{
    std::wcout << L"[OBSHelper] 🚀 Attempting to start OBS Virtual Camera..." << std::endl;
    
    if (!IsOBSVirtualCameraInstalled()) {
        ShowOBSInstallationGuide();
        return false;
    }
    
    // Check if OBS is running
    if (!IsOBSRunning()) {
        MessageBoxA(nullptr,
            "⚠️ OBS Studio Not Running\n\n"
            "OBS Virtual Camera is installed but OBS Studio is not running.\n\n"
            "📋 To use virtual camera:\n"
            "1. Open OBS Studio\n"
            "2. Set up your scene (add camera source)\n"
            "3. Click 'Start Virtual Camera' button\n"
            "4. Virtual camera will be available in other apps\n\n"
            "💡 MySubstitute can then process your camera feed!",
            "OBS Not Running", MB_OK | MB_ICONINFORMATION);
        return false;
    }
    
    MessageBoxA(nullptr,
        "🎭 OBS Virtual Camera Available!\n\n"
        "✅ OBS Studio is running with virtual camera support.\n\n"
        "📋 How to activate:\n"
        "1. In OBS Studio, click 'Start Virtual Camera' button\n"
        "2. Virtual camera will appear in Zoom, Teams, etc.\n"
        "3. Use MySubstitute to enhance your real camera\n"
        "4. OBS handles the virtual camera output\n\n"
        "🔄 This is the recommended setup for reliable virtual camera!",
        "Virtual Camera Ready", MB_OK | MB_ICONINFORMATION);
    
    return true;
}

void OBSVirtualCameraHelper::ShowOBSSetupInstructions()
{
    MessageBoxA(nullptr,
        "📋 COMPLETE VIRTUAL CAMERA SETUP\n\n"
        "🎯 Recommended workflow:\n\n"
        "1️⃣ INSTALL OBS STUDIO (if not installed)\n"
        "   • Download from https://obsproject.com/\n"
        "   • Free and includes virtual camera\n\n"
        "2️⃣ SETUP OBS VIRTUAL CAMERA\n"
        "   • Open OBS Studio\n"
        "   • Add your camera as a source\n"
        "   • Click 'Start Virtual Camera'\n\n"
        "3️⃣ USE MYSUBSTITUTE\n"
        "   • Start MySubstitute camera processing\n"
        "   • AI effects applied to your real camera\n"
        "   • OBS virtual camera shows processed video\n\n"
        "4️⃣ SELECT IN APPS\n"
        "   • Choose 'OBS Virtual Camera' in Zoom, Teams\n"
        "   • Enjoy AI-enhanced video calls!\n\n"
        "💡 This setup gives you the most reliable virtual camera experience.",
        "Complete Setup Guide", MB_OK | MB_ICONINFORMATION);
}

bool OBSVirtualCameraHelper::CanIntegrateWithOBS()
{
    return IsOBSVirtualCameraInstalled() && IsOBSRunning();
}

bool OBSVirtualCameraHelper::CheckOBSRegistry()
{
    // Check common OBS registry locations
    HKEY hKey;
    
    // Check for OBS Virtual Camera CLSID
    LONG result = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\Classes\\CLSID\\{27B05C2D-93DC-474A-A5DA-9BBA34CB2A9C}",
        0, KEY_READ, &hKey);
    
    if (result == ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return true;
    }
    
    // Check for OBS installation
    result = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\OBS Studio",
        0, KEY_READ, &hKey);
    
    if (result == ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return true;
    }
    
    // Check current user registry
    result = RegOpenKeyExW(HKEY_CURRENT_USER,
        L"SOFTWARE\\Classes\\CLSID\\{27B05C2D-93DC-474A-A5DA-9BBA34CB2A9C}",
        0, KEY_READ, &hKey);
    
    if (result == ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return true;
    }
    
    return false;
}

bool OBSVirtualCameraHelper::CheckOBSProcess()
{
    HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE) {
        return false;
    }
    
    PROCESSENTRY32W pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32W);
    
    bool found = false;
    if (Process32FirstW(hProcessSnap, &pe32)) {
        do {
            std::wstring processName = pe32.szExeFile;
            if (processName == L"obs64.exe" || 
                processName == L"obs32.exe" || 
                processName == L"obs.exe") {
                found = true;
                break;
            }
        } while (Process32NextW(hProcessSnap, &pe32));
    }
    
    CloseHandle(hProcessSnap);
    return found;
}

std::wstring OBSVirtualCameraHelper::GetOBSInstallPath()
{
    HKEY hKey;
    LONG result = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\OBS Studio",
        0, KEY_READ, &hKey);
    
    if (result == ERROR_SUCCESS) {
        DWORD dataSize = 0;
        DWORD dataType = 0;
        result = RegQueryValueExW(hKey, L"InstallLocation", nullptr, &dataType, nullptr, &dataSize);
        
        if (result == ERROR_SUCCESS && dataSize > 0) {
            std::vector<wchar_t> buffer(dataSize / sizeof(wchar_t));
            result = RegQueryValueExW(hKey, L"InstallLocation", nullptr, &dataType,
                                    (LPBYTE)buffer.data(), &dataSize);
            
            if (result == ERROR_SUCCESS) {
                RegCloseKey(hKey);
                return std::wstring(buffer.data());
            }
        }
        RegCloseKey(hKey);
    }
    
    return L"";
}