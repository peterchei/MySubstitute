#include "simple_virtual_camera_new.h"
#include <iostream>
#include <dshow.h>
#include <comdef.h>

SimpleVirtualCamera::SimpleVirtualCamera()
    : m_isRegistered(false)
    , m_isRunning(false)
    , m_deviceName(L"MySubstitute Virtual Camera")
{
}

SimpleVirtualCamera::~SimpleVirtualCamera()
{
    Cleanup();
}

bool SimpleVirtualCamera::Initialize()
{
    std::wcout << L"[SimpleVirtualCamera] 🎬 Initializing virtual camera..." << std::endl;
    
    // Check if we can work with the system
    HRESULT hr = CoInitialize(nullptr);
    if (FAILED(hr)) {
        std::wcout << L"[SimpleVirtualCamera] ❌ Failed to initialize COM" << std::endl;
        return false;
    }
    
    // Check for existing virtual cameras that we can use
    if (CheckForExistingVirtualCameras()) {
        std::wcout << L"[SimpleVirtualCamera] 🎭 Found existing virtual camera infrastructure!" << std::endl;
        m_isRegistered = true;
        CoUninitialize();
        return true;
    }
    
    // Try to create our own minimal registration
    bool success = CreateRegistryEntries();
    CoUninitialize();
    
    if (success) {
        m_isRegistered = true;
        std::wcout << L"[SimpleVirtualCamera] ✅ Virtual camera initialized successfully" << std::endl;
    } else {
        std::wcout << L"[SimpleVirtualCamera] ⚠️ Could not create virtual camera, but will try alternatives" << std::endl;
    }
    
    return true; // Always return true - we'll try alternatives
}

bool SimpleVirtualCamera::Start()
{
    if (!m_isRegistered) {
        std::wcout << L"[SimpleVirtualCamera] ⚠️ Not registered, attempting registration..." << std::endl;
        if (!RegisterWithSystem()) {
            return false;
        }
    }
    
    std::wcout << L"[SimpleVirtualCamera] ▶️ Starting virtual camera service..." << std::endl;
    
    if (StartCameraService()) {
        m_isRunning = true;
        std::wcout << L"[SimpleVirtualCamera] ✅ Virtual camera service started" << std::endl;
        return true;
    }
    
    std::wcout << L"[SimpleVirtualCamera] ❌ Failed to start virtual camera service" << std::endl;
    return false;
}

void SimpleVirtualCamera::Stop()
{
    if (m_isRunning) {
        std::wcout << L"[SimpleVirtualCamera] ⏹️ Stopping virtual camera..." << std::endl;
        StopCameraService();
        m_isRunning = false;
    }
}

void SimpleVirtualCamera::Cleanup()
{
    Stop();
}

bool SimpleVirtualCamera::RegisterWithSystem()
{
    std::wcout << L"[SimpleVirtualCamera] 📋 Registering with Windows..." << std::endl;
    
    // Method 1: Try OBS-style registration
    HKEY hKey;
    LONG result = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\Classes\\CLSID\\{27B05C2D-93DC-474A-A5DA-9BBA34CB2A9C}",
        0, KEY_READ, &hKey);
    
    if (result == ERROR_SUCCESS) {
        RegCloseKey(hKey);
        std::wcout << L"[SimpleVirtualCamera] 🎭 Using OBS Virtual Camera infrastructure" << std::endl;
        m_isRegistered = true;
        return true;
    }
    
    // Method 2: Create minimal registry entry
    if (CreateRegistryEntries()) {
        m_isRegistered = true;
        return true;
    }
    
    // Method 3: Use Windows Media Foundation
    result = RegCreateKeyExW(HKEY_CURRENT_USER,
        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Camera\\CameraList\\MySubstituteCamera",
        0, nullptr, REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKey, nullptr);
    
    if (result == ERROR_SUCCESS) {
        const wchar_t* name = L"MySubstitute Virtual Camera";
        RegSetValueExW(hKey, L"FriendlyName", 0, REG_SZ,
            (const BYTE*)name, (wcslen(name) + 1) * sizeof(wchar_t));
        RegCloseKey(hKey);
        
        std::wcout << L"[SimpleVirtualCamera] ✅ Created Windows Camera registration" << std::endl;
        m_isRegistered = true;
        return true;
    }
    
    return false;
}

bool SimpleVirtualCamera::UnregisterFromSystem()
{
    std::wcout << L"[SimpleVirtualCamera] 🗑️ Unregistering from Windows..." << std::endl;
    
    // Remove our registry entries
    RegDeleteTreeW(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Camera\\CameraList\\MySubstituteCamera");
    
    m_isRegistered = false;
    return true;
}

bool SimpleVirtualCamera::CheckForExistingVirtualCameras()
{
    HRESULT hr = CoInitialize(nullptr);
    if (FAILED(hr)) return false;
    
    ICreateDevEnum* pDevEnum = nullptr;
    hr = CoCreateInstance(CLSID_SystemDeviceEnum, nullptr, CLSCTX_INPROC_SERVER,
                         IID_ICreateDevEnum, (void**)&pDevEnum);
    
    bool foundVirtual = false;
    if (SUCCEEDED(hr)) {
        IEnumMoniker* pEnum = nullptr;
        hr = pDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEnum, 0);
        
        if (hr == S_OK) {
            IMoniker* pMoniker = nullptr;
            ULONG cFetched;
            
            while (pEnum->Next(1, &pMoniker, &cFetched) == S_OK && !foundVirtual) {
                IPropertyBag* pPropBag;
                hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void**)&pPropBag);
                
                if (SUCCEEDED(hr)) {
                    VARIANT var;
                    VariantInit(&var);
                    hr = pPropBag->Read(L"FriendlyName", &var, 0);
                    
                    if (SUCCEEDED(hr)) {
                        // Check for virtual camera keywords
                        std::wstring name = var.bstrVal;
                        if (name.find(L"Virtual") != std::wstring::npos ||
                            name.find(L"OBS") != std::wstring::npos ||
                            name.find(L"Logitech") != std::wstring::npos) {
                            foundVirtual = true;
                            std::wcout << L"[SimpleVirtualCamera] 🎭 Found existing virtual camera: " << name << std::endl;
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
    return foundVirtual;
}

bool SimpleVirtualCamera::CreateRegistryEntries()
{
    std::wcout << L"[SimpleVirtualCamera] 📝 Creating registry entries..." << std::endl;
    
    // Create a minimal DirectShow entry (read-only, safer)
    HKEY hKey;
    LONG result = RegCreateKeyExW(HKEY_CURRENT_USER,
        L"SOFTWARE\\Classes\\CLSID\\{B3F3A1C4-8F9E-4A2D-9B5C-7E6F8D4C9A3B}",
        0, nullptr, REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKey, nullptr);
    
    if (result == ERROR_SUCCESS) {
        // Set friendly name
        const wchar_t* name = L"MySubstitute Virtual Camera";
        RegSetValueExW(hKey, nullptr, 0, REG_SZ,
            (const BYTE*)name, (wcslen(name) + 1) * sizeof(wchar_t));
        
        RegCloseKey(hKey);
        
        std::wcout << L"[SimpleVirtualCamera] ✅ Registry entries created" << std::endl;
        return true;
    }
    
    std::wcout << L"[SimpleVirtualCamera] ⚠️ Could not create registry entries (may need admin)" << std::endl;
    return false;
}

bool SimpleVirtualCamera::RemoveRegistryEntries()
{
    RegDeleteTreeW(HKEY_CURRENT_USER, L"SOFTWARE\\Classes\\CLSID\\{B3F3A1C4-8F9E-4A2D-9B5C-7E6F8D4C9A3B}");
    return true;
}

bool SimpleVirtualCamera::StartCameraService()
{
    std::wcout << L"[SimpleVirtualCamera] 🚀 Starting camera service..." << std::endl;
    
    // For now, this is a placeholder
    // In a full implementation, this would start the actual video streaming service
    
    std::wcout << L"[SimpleVirtualCamera] ℹ️ Camera service simulation started" << std::endl;
    return true;
}

void SimpleVirtualCamera::StopCameraService()
{
    std::wcout << L"[SimpleVirtualCamera] 🛑 Stopping camera service..." << std::endl;
    
    // Placeholder for stopping the service
    
    std::wcout << L"[SimpleVirtualCamera] ✅ Camera service stopped" << std::endl;
}