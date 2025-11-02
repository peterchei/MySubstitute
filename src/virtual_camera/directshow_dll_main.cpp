#include "virtual_camera_directshow.h"
#include <windows.h>
#include <olectl.h>
#include <initguid.h>
#include <iostream>

//
// DLL Entry Point and COM Registration
//

HINSTANCE g_hInst = nullptr;

// DLL Entry Point
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        g_hInst = hModule;
        DisableThreadLibraryCalls(hModule);
        OutputDebugStringA("[MySubstitute DLL] Process attach\n");
        break;
        
    case DLL_PROCESS_DETACH:
        OutputDebugStringA("[MySubstitute DLL] Process detach\n");
        break;
    }
    return TRUE;
}

//
// COM Class Factory Implementation
//

class MySubstituteClassFactory : public IClassFactory
{
private:
    volatile LONG m_cRef;
    
public:
    MySubstituteClassFactory() : m_cRef(1) {}
    virtual ~MySubstituteClassFactory() {}
    
    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv) {
        if (riid == IID_IUnknown || riid == IID_IClassFactory) {
            *ppv = this;
            AddRef();
            return S_OK;
        }
        *ppv = nullptr;
        return E_NOINTERFACE;
    }
    
    STDMETHODIMP_(ULONG) AddRef() {
        return InterlockedIncrement(&m_cRef);
    }
    
    STDMETHODIMP_(ULONG) Release() {
        ULONG cRef = InterlockedDecrement(&m_cRef);
        if (cRef == 0) {
            delete this;
        }
        return cRef;
    }
    
    // IClassFactory
    STDMETHODIMP CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppv) {
        if (pUnkOuter != nullptr) {
            return CLASS_E_NOAGGREGATION;
        }
        
        MySubstituteVirtualCameraFilter* pFilter = MySubstituteVirtualCameraFilter::CreateInstance();
        if (!pFilter) {
            return E_OUTOFMEMORY;
        }
        
        HRESULT hr = pFilter->QueryInterface(riid, ppv);
        pFilter->Release();
        
        OutputDebugStringA("[MySubstitute DLL] Filter instance created\n");
        
        return hr;
    }
    
    STDMETHODIMP LockServer(BOOL fLock) {
        return S_OK;
    }
};

//
// DLL Export Functions Required by COM
//

extern "C" {

// Can the DLL be unloaded?
STDAPI DllCanUnloadNow(void)
{
    // For simplicity, always allow unloading
    // In production, you'd track object references
    return S_OK;
}

// Get class factory
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    *ppv = nullptr;
    
    if (rclsid != CLSID_MySubstituteVirtualCamera) {
        return CLASS_E_CLASSNOTAVAILABLE;
    }
    
    MySubstituteClassFactory* pFactory = new MySubstituteClassFactory();
    if (!pFactory) {
        return E_OUTOFMEMORY;
    }
    
    HRESULT hr = pFactory->QueryInterface(riid, ppv);
    pFactory->Release();
    
    return hr;
}

// Register the COM server
STDAPI DllRegisterServer(void)
{
    HKEY hKey;
    LONG result;
    WCHAR modulePath[MAX_PATH];
    
    if (!GetModuleFileNameW(g_hInst, modulePath, MAX_PATH)) {
        return HRESULT_FROM_WIN32(GetLastError());
    }
    
    std::wcout << L"[DirectShow DLL] Registering: " << modulePath << std::endl;
    
    // Register CLSID in HKEY_CLASSES_ROOT
    std::wstring clsidKey = L"CLSID\\{B3F3A1C4-8F9E-4A2D-9B5C-7E6F8D4C9A3B}";
    result = RegCreateKeyExW(HKEY_CLASSES_ROOT, clsidKey.c_str(), 0, nullptr, 
                           REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKey, nullptr);
    if (result != ERROR_SUCCESS) return HRESULT_FROM_WIN32(result);
    
    // Friendly name
    std::wstring friendlyName = L"MySubstitute Virtual Camera";
    RegSetValueExW(hKey, nullptr, 0, REG_SZ, (BYTE*)friendlyName.c_str(), 
                  (DWORD)((friendlyName.length() + 1) * sizeof(wchar_t)));
    RegCloseKey(hKey);
    
    // InprocServer32 - Point to our DLL
    std::wstring inprocKey = clsidKey + L"\\InprocServer32";
    result = RegCreateKeyExW(HKEY_CLASSES_ROOT, inprocKey.c_str(), 0, nullptr,
                           REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKey, nullptr);
    if (result != ERROR_SUCCESS) return HRESULT_FROM_WIN32(result);
    
    RegSetValueExW(hKey, nullptr, 0, REG_SZ, (BYTE*)modulePath, 
                  (DWORD)((wcslen(modulePath) + 1) * sizeof(wchar_t)));
    RegSetValueExW(hKey, L"ThreadingModel", 0, REG_SZ, (BYTE*)L"Both", 5 * sizeof(wchar_t));
    RegCloseKey(hKey);
    
    // Register in DirectShow video input device category
    // CLSID_VideoInputDeviceCategory = {860BB310-5D01-11d0-BD3B-00A0C911CE86}
    std::wstring categoryKey = L"CLSID\\{860BB310-5D01-11d0-BD3B-00A0C911CE86}\\Instance\\{B3F3A1C4-8F9E-4A2D-9B5C-7E6F8D4C9A3B}";
    result = RegCreateKeyExW(HKEY_CLASSES_ROOT, categoryKey.c_str(), 0, nullptr,
                           REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKey, nullptr);
    if (result != ERROR_SUCCESS) return HRESULT_FROM_WIN32(result);
    
    RegSetValueExW(hKey, L"FriendlyName", 0, REG_SZ, (BYTE*)friendlyName.c_str(),
                  (DWORD)((friendlyName.length() + 1) * sizeof(wchar_t)));
    
    // CLSID value
    std::wstring clsidValue = L"{B3F3A1C4-8F9E-4A2D-9B5C-7E6F8D4C9A3B}";
    RegSetValueExW(hKey, L"CLSID", 0, REG_SZ, (BYTE*)clsidValue.c_str(),
                  (DWORD)((clsidValue.length() + 1) * sizeof(wchar_t)));
    
    RegCloseKey(hKey);
    
    std::wcout << L"[DirectShow DLL] ✅ Registration completed successfully!" << std::endl;
    return S_OK;
}

// Unregister the COM server
STDAPI DllUnregisterServer(void)
{
    std::wcout << L"[DirectShow DLL] Unregistering virtual camera..." << std::endl;
    
    // Delete from video input device category
    RegDeleteTreeW(HKEY_CLASSES_ROOT, 
        L"CLSID\\{860BB310-5D01-11d0-BD3B-00A0C911CE86}\\Instance\\{B3F3A1C4-8F9E-4A2D-9B5C-7E6F8D4C9A3B}");
    
    // Delete CLSID registration
    RegDeleteTreeW(HKEY_CLASSES_ROOT, L"CLSID\\{B3F3A1C4-8F9E-4A2D-9B5C-7E6F8D4C9A3B}");
    
    std::wcout << L"[DirectShow DLL] ✅ Unregistration completed!" << std::endl;
    return S_OK;
}

} // extern "C"