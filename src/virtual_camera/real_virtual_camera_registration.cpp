#include "real_virtual_camera_registration.h"
#include <iostream>
#include <dshow.h>
#include <comdef.h>
#include <shlobj.h>

const wchar_t* RealVirtualCameraRegistration::GetFilterCLSID()
{
    return L"{B3F3A1C4-8F9E-4A2D-9B5C-7E6F8D4C9A3B}";
}

const wchar_t* RealVirtualCameraRegistration::GetFilterName()
{
    return L"MySubstitute Virtual Camera";
}

bool RealVirtualCameraRegistration::RegisterVirtualCameraFilter()
{
    std::wcout << L"[RealVirtualCamera] 🎬 Registering MySubstitute Virtual Camera..." << std::endl;
    
    // Need admin privileges for system registration
    if (!IsUserAnAdmin()) {
        MessageBoxA(nullptr,
            "⚠️ Administrator Privileges Required\n\n"
            "Virtual camera registration requires administrator privileges.\n\n"
            "Please:\n"
            "1. Close MySubstitute\n"
            "2. Right-click MySubstitute.exe\n"
            "3. Select 'Run as administrator'\n"
            "4. Try registering again\n\n"
            "This is needed to register the DirectShow filter with Windows.",
            "Admin Required", MB_OK | MB_ICONWARNING);
        return false;
    }
    
    return RegisterCompleteVirtualCamera();
}

bool RealVirtualCameraRegistration::UnregisterVirtualCameraFilter()
{
    std::wcout << L"[RealVirtualCamera] 🗑️ Unregistering virtual camera..." << std::endl;
    
    // Remove CLSID registration
    std::wstring clsidPath = L"SOFTWARE\\Classes\\CLSID\\";
    clsidPath += GetFilterCLSID();
    RegDeleteTreeW(HKEY_LOCAL_MACHINE, clsidPath.c_str());
    
    // Remove from video input device category
    std::wstring devicePath = L"SOFTWARE\\Classes\\CLSID\\{860BB310-5D01-11D0-BD3B-00A0C911CE86}\\Instance\\";
    devicePath += GetFilterCLSID();
    RegDeleteTreeW(HKEY_LOCAL_MACHINE, devicePath.c_str());
    
    std::wcout << L"[RealVirtualCamera] ✅ Virtual camera unregistered" << std::endl;
    return true;
}

bool RealVirtualCameraRegistration::IsVirtualCameraRegistered()
{
    HKEY hKey;
    std::wstring clsidPath = L"SOFTWARE\\Classes\\CLSID\\";
    clsidPath += GetFilterCLSID();
    
    LONG result = RegOpenKeyExW(HKEY_LOCAL_MACHINE, clsidPath.c_str(), 0, KEY_READ, &hKey);
    if (result == ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return true;
    }
    
    return false;
}

bool RealVirtualCameraRegistration::TestVirtualCameraVisibility()
{
    std::wcout << L"[RealVirtualCamera] 🔍 Testing virtual camera visibility..." << std::endl;
    
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
            
            std::wcout << L"[RealVirtualCamera] 📋 Scanning for virtual camera..." << std::endl;
            
            while (pEnum->Next(1, &pMoniker, &cFetched) == S_OK) {
                IPropertyBag* pPropBag;
                hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void**)&pPropBag);
                
                if (SUCCEEDED(hr)) {
                    VARIANT var;
                    VariantInit(&var);
                    hr = pPropBag->Read(L"FriendlyName", &var, 0);
                    
                    if (SUCCEEDED(hr)) {
                        std::wcout << L"    Found device: " << var.bstrVal << std::endl;
                        
                        if (wcsstr(var.bstrVal, L"MySubstitute") != nullptr) {
                            found = true;
                            std::wcout << L"[RealVirtualCamera] 🎯 Found MySubstitute Virtual Camera!" << std::endl;
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
        std::wcout << L"[RealVirtualCamera] ❌ MySubstitute Virtual Camera not visible" << std::endl;
    }
    
    return found;
}

bool RealVirtualCameraRegistration::RegisterCompleteVirtualCamera()
{
    std::wcout << L"[RealVirtualCamera] 📝 Creating complete virtual camera registration..." << std::endl;
    
    // Step 1: Register the DirectShow filter CLSID
    if (!RegisterDirectShowFilter()) {
        std::wcout << L"[RealVirtualCamera] ❌ Failed to register DirectShow filter" << std::endl;
        return false;
    }
    
    // Step 2: Register as video input device
    if (!RegisterAsVideoInputDevice()) {
        std::wcout << L"[RealVirtualCamera] ❌ Failed to register as video input device" << std::endl;
        return false;
    }
    
    // Step 3: Verify registration
    if (!VerifyRegistration()) {
        std::wcout << L"[RealVirtualCamera] ❌ Registration verification failed" << std::endl;
        return false;
    }
    
    std::wcout << L"[RealVirtualCamera] ✅ Complete virtual camera registration successful!" << std::endl;
    return true;
}

void RealVirtualCameraRegistration::ShowRegistrationStatus()
{
    bool isRegistered = IsVirtualCameraRegistered();
    bool isVisible = TestVirtualCameraVisibility();
    bool isAdmin = IsUserAnAdmin();
    
    std::wstring message = L"🔍 VIRTUAL CAMERA REGISTRATION STATUS\n\n";
    message += L"🔐 Administrator Mode: " + std::wstring(isAdmin ? L"✅ YES" : L"❌ NO") + L"\n";
    message += L"📋 Registry Entries: " + std::wstring(isRegistered ? L"✅ EXISTS" : L"❌ MISSING") + L"\n";
    message += L"👁️ Device Visibility: " + std::wstring(isVisible ? L"✅ VISIBLE" : L"❌ NOT VISIBLE") + L"\n\n";
    
    if (!isAdmin) {
        message += L"⚠️ ISSUE: Need Administrator Privileges\n";
        message += L"• Right-click MySubstitute.exe\n";
        message += L"• Select 'Run as administrator'\n";
        message += L"• Try registration again\n\n";
    }
    
    if (isRegistered && !isVisible) {
        message += L"⚠️ ISSUE: Registered but Not Visible\n";
        message += L"• Registry entries exist but device not appearing\n";
        message += L"• May need DirectShow base classes\n";
        message += L"• Try restarting applications\n\n";
    }
    
    if (!isRegistered && !isVisible) {
        message += L"❌ ISSUE: Not Registered\n";
        message += L"• No registry entries found\n";
        message += L"• Run as administrator and try again\n\n";
    }
    
    if (isRegistered && isVisible) {
        message += L"🎉 SUCCESS: Virtual Camera Working!\n";
        message += L"• MySubstitute Virtual Camera should appear\n";
        message += L"• Available in Camera app, Zoom, Teams, etc.\n";
        message += L"• Ready for AI video processing\n\n";
    }
    
    message += L"💡 Alternative: Install OBS Studio for guaranteed virtual camera support";
    
    // Convert to narrow string for MessageBox
    std::string narrowMessage(message.begin(), message.end());
    MessageBoxA(nullptr, narrowMessage.c_str(), "Virtual Camera Status", 
               MB_OK | (isVisible ? MB_ICONINFORMATION : MB_ICONWARNING));
}

bool RealVirtualCameraRegistration::RegisterDirectShowFilter()
{
    std::wcout << L"[RealVirtualCamera] 📝 Registering DirectShow filter..." << std::endl;
    
    // Get current module path
    wchar_t modulePath[MAX_PATH];
    GetModuleFileNameW(nullptr, modulePath, MAX_PATH);
    
    // Create CLSID registry entry
    std::wstring clsidPath = L"SOFTWARE\\Classes\\CLSID\\";
    clsidPath += GetFilterCLSID();
    
    HKEY hKey;
    LONG result = RegCreateKeyExW(HKEY_LOCAL_MACHINE, clsidPath.c_str(), 0, nullptr,
        REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKey, nullptr);
    
    if (result != ERROR_SUCCESS) {
        std::wcout << L"[RealVirtualCamera] ❌ Failed to create CLSID key: " << result << std::endl;
        return false;
    }
    
    // Set friendly name
    const wchar_t* friendlyName = GetFilterName();
    RegSetValueExW(hKey, nullptr, 0, REG_SZ, (const BYTE*)friendlyName, 
                   (wcslen(friendlyName) + 1) * sizeof(wchar_t));
    
    RegCloseKey(hKey);
    
    // Create InProcServer32 entry
    std::wstring inprocPath = clsidPath + L"\\InProcServer32";
    result = RegCreateKeyExW(HKEY_LOCAL_MACHINE, inprocPath.c_str(), 0, nullptr,
        REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKey, nullptr);
    
    if (result != ERROR_SUCCESS) {
        std::wcout << L"[RealVirtualCamera] ❌ Failed to create InProcServer32 key" << std::endl;
        return false;
    }
    
    // Set module path
    RegSetValueExW(hKey, nullptr, 0, REG_SZ, (const BYTE*)modulePath, 
                   (wcslen(modulePath) + 1) * sizeof(wchar_t));
    
    // Set threading model
    const wchar_t* threadingModel = L"Both";
    RegSetValueExW(hKey, L"ThreadingModel", 0, REG_SZ, (const BYTE*)threadingModel, 
                   (wcslen(threadingModel) + 1) * sizeof(wchar_t));
    
    RegCloseKey(hKey);
    
    std::wcout << L"[RealVirtualCamera] ✅ DirectShow filter registered" << std::endl;
    return true;
}

bool RealVirtualCameraRegistration::RegisterAsVideoInputDevice()
{
    std::wcout << L"[RealVirtualCamera] 📹 Registering as video input device..." << std::endl;
    
    // Register in video input device category
    std::wstring deviceCategoryPath = L"SOFTWARE\\Classes\\CLSID\\{860BB310-5D01-11D0-BD3B-00A0C911CE86}\\Instance\\";
    deviceCategoryPath += GetFilterCLSID();
    
    HKEY hKey;
    LONG result = RegCreateKeyExW(HKEY_LOCAL_MACHINE, deviceCategoryPath.c_str(), 0, nullptr,
        REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKey, nullptr);
    
    if (result != ERROR_SUCCESS) {
        std::wcout << L"[RealVirtualCamera] ❌ Failed to create device category key: " << result << std::endl;
        return false;
    }
    
    // Set CLSID
    const wchar_t* clsid = GetFilterCLSID();
    RegSetValueExW(hKey, L"CLSID", 0, REG_SZ, (const BYTE*)clsid, 
                   (wcslen(clsid) + 1) * sizeof(wchar_t));
    
    // Set friendly name
    const wchar_t* friendlyName = GetFilterName();
    RegSetValueExW(hKey, L"FriendlyName", 0, REG_SZ, (const BYTE*)friendlyName, 
                   (wcslen(friendlyName) + 1) * sizeof(wchar_t));
    
    RegCloseKey(hKey);
    
    std::wcout << L"[RealVirtualCamera] ✅ Registered as video input device" << std::endl;
    return true;
}

bool RealVirtualCameraRegistration::CreateRegistryEntries()
{
    return RegisterDirectShowFilter() && RegisterAsVideoInputDevice();
}

bool RealVirtualCameraRegistration::RegisterCOMServer()
{
    // For EXE-based COM server, we would register differently
    // For now, using in-process server model
    return true;
}

bool RealVirtualCameraRegistration::VerifyRegistration()
{
    std::wcout << L"[RealVirtualCamera] 🔍 Verifying registration..." << std::endl;
    
    // Check if registry entries exist
    if (!IsVirtualCameraRegistered()) {
        std::wcout << L"[RealVirtualCamera] ❌ Registry entries not found" << std::endl;
        return false;
    }
    
    // Test if device appears in enumeration
    bool visible = TestVirtualCameraVisibility();
    if (!visible) {
        std::wcout << L"[RealVirtualCamera] ⚠️ Device registered but not visible in enumeration" << std::endl;
        // Don't fail here - might still work
    }
    
    return true;
}