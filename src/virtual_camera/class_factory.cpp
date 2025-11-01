#include "class_factory.h"
#include "virtual_camera_directshow.h"
#include <strsafe.h>
#include <iostream>

// Global server lock count
LONG g_cServerLocks = 0;

//=============================================================================
// MySubstituteClassFactory Implementation
//=============================================================================

MySubstituteClassFactory::MySubstituteClassFactory() : m_cRef(1)
{
}

MySubstituteClassFactory::~MySubstituteClassFactory()
{
}

// IUnknown methods
STDMETHODIMP MySubstituteClassFactory::QueryInterface(REFIID riid, void **ppv)
{
    if (!ppv) return E_POINTER;
    
    if (riid == IID_IUnknown || riid == IID_IClassFactory) {
        *ppv = this;
        AddRef();
        return S_OK;
    }
    
    *ppv = nullptr;
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) MySubstituteClassFactory::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) MySubstituteClassFactory::Release()
{
    LONG cRef = InterlockedDecrement(&m_cRef);
    if (cRef == 0) {
        delete this;
    }
    return cRef;
}

// IClassFactory methods
STDMETHODIMP MySubstituteClassFactory::CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppv)
{
    if (!ppv) return E_POINTER;
    
    if (pUnkOuter) return CLASS_E_NOAGGREGATION;
    
    MySubstituteVirtualCameraFilter* pFilter = MySubstituteVirtualCameraFilter::CreateInstance();
    if (!pFilter) return E_OUTOFMEMORY;
    
    HRESULT hr = pFilter->QueryInterface(riid, ppv);
    pFilter->Release(); // Release our reference, QI adds its own
    
    return hr;
}

STDMETHODIMP MySubstituteClassFactory::LockServer(BOOL fLock)
{
    if (fLock) {
        InterlockedIncrement(&g_cServerLocks);
    } else {
        InterlockedDecrement(&g_cServerLocks);
    }
    return S_OK;
}

//=============================================================================
// DLL Export Functions
//=============================================================================

STDAPI DllCanUnloadNow()
{
    return (g_cServerLocks == 0) ? S_OK : S_FALSE;
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    if (!ppv) return E_POINTER;
    
    if (rclsid == CLSID_MySubstituteVirtualCamera) {
        MySubstituteClassFactory* pFactory = new MySubstituteClassFactory();
        if (!pFactory) return E_OUTOFMEMORY;
        
        HRESULT hr = pFactory->QueryInterface(riid, ppv);
        pFactory->Release();
        return hr;
    }
    
    *ppv = nullptr;
    return CLASS_E_CLASSNOTAVAILABLE;
}

STDAPI DllRegisterServer()
{
    std::wcout << L"[MySubstitute] Registering virtual camera..." << std::endl;
    
    HKEY hKey;
    LONG result;
    WCHAR szCLSID[64];
    WCHAR szSubkey[256];
    WCHAR szModule[MAX_PATH];
    
    // Convert CLSID to string
    StringFromGUID2(CLSID_MySubstituteVirtualCamera, szCLSID, 64);
    
    // Get current module path
    GetModuleFileNameW(GetModuleHandle(nullptr), szModule, MAX_PATH);
    
    // Register CLSID
    StringCchPrintfW(szSubkey, 256, L"CLSID\\%s", szCLSID);
    result = RegCreateKeyExW(HKEY_CLASSES_ROOT, szSubkey, 0, nullptr, 
        REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKey, nullptr);
    
    if (result == ERROR_SUCCESS) {
        RegSetValueExW(hKey, nullptr, 0, REG_SZ, 
            (BYTE*)L"MySubstitute Virtual Camera", 
            (wcslen(L"MySubstitute Virtual Camera") + 1) * sizeof(WCHAR));
        RegCloseKey(hKey);
    } else {
        std::wcout << L"[MySubstitute] Failed to create CLSID registry key: " << result << std::endl;
        return SELFREG_E_CLASS;
    }
    
    // Register InprocServer32
    StringCchPrintfW(szSubkey, 256, L"CLSID\\%s\\InprocServer32", szCLSID);
    result = RegCreateKeyExW(HKEY_CLASSES_ROOT, szSubkey, 0, nullptr,
        REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKey, nullptr);
    
    if (result == ERROR_SUCCESS) {
        RegSetValueExW(hKey, nullptr, 0, REG_SZ, (BYTE*)szModule, 
            (wcslen(szModule) + 1) * sizeof(WCHAR));
        RegSetValueExW(hKey, L"ThreadingModel", 0, REG_SZ, 
            (BYTE*)L"Both", (wcslen(L"Both") + 1) * sizeof(WCHAR));
        RegCloseKey(hKey);
    } else {
        std::wcout << L"[MySubstitute] Failed to create InprocServer32 registry key: " << result << std::endl;
        return SELFREG_E_CLASS;
    }
    
    // Register as DirectShow filter
    StringCchPrintfW(szSubkey, 256, L"CLSID\\%s\\Instance\\{083863F1-70DE-11D0-BD40-00A0C911CE86}\\CLSID", szCLSID);
    result = RegCreateKeyExW(HKEY_CLASSES_ROOT, szSubkey, 0, nullptr,
        REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKey, nullptr);
    
    if (result == ERROR_SUCCESS) {
        RegSetValueExW(hKey, nullptr, 0, REG_SZ, (BYTE*)szCLSID, 
            (wcslen(szCLSID) + 1) * sizeof(WCHAR));
        RegCloseKey(hKey);
    }
    
    // Register in DirectShow category for Video Capture Sources
    // CLSID_VideoInputDeviceCategory = {860BB310-5D01-11D0-BD3B-00A0C911CE86}
    WCHAR szCategoryKey[256];
    StringCchPrintfW(szCategoryKey, 256, 
        L"CLSID\\{860BB310-5D01-11D0-BD3B-00A0C911CE86}\\Instance\\%s", szCLSID);
    
    result = RegCreateKeyExW(HKEY_CLASSES_ROOT, szCategoryKey, 0, nullptr,
        REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKey, nullptr);
    
    if (result == ERROR_SUCCESS) {
        RegSetValueExW(hKey, L"CLSID", 0, REG_SZ, (BYTE*)szCLSID,
            (wcslen(szCLSID) + 1) * sizeof(WCHAR));
        RegSetValueExW(hKey, L"FriendlyName", 0, REG_SZ, 
            (BYTE*)L"MySubstitute Virtual Camera",
            (wcslen(L"MySubstitute Virtual Camera") + 1) * sizeof(WCHAR));
        RegCloseKey(hKey);
        
        std::wcout << L"[MySubstitute] Virtual camera registered successfully!" << std::endl;
        std::wcout << L"[MySubstitute] CLSID: " << szCLSID << std::endl;
        return S_OK;
    } else {
        std::wcout << L"[MySubstitute] Failed to register in video capture category: " << result << std::endl;
        return SELFREG_E_CLASS;
    }
}

STDAPI DllUnregisterServer()
{
    std::wcout << L"[MySubstitute] Unregistering virtual camera..." << std::endl;
    
    WCHAR szCLSID[64];
    WCHAR szSubkey[256];
    
    // Convert CLSID to string
    StringFromGUID2(CLSID_MySubstituteVirtualCamera, szCLSID, 64);
    
    // Remove from video capture category
    StringCchPrintfW(szSubkey, 256, 
        L"CLSID\\{860BB310-5D01-11D0-BD3B-00A0C911CE86}\\Instance\\%s", szCLSID);
    RegDeleteTreeW(HKEY_CLASSES_ROOT, szSubkey);
    
    // Remove CLSID registration
    StringCchPrintfW(szSubkey, 256, L"CLSID\\%s", szCLSID);
    RegDeleteTreeW(HKEY_CLASSES_ROOT, szSubkey);
    
    std::wcout << L"[MySubstitute] Virtual camera unregistered successfully!" << std::endl;
    return S_OK;
}