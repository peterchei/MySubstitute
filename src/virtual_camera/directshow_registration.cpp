#include "directshow_source_filter.h"
#include <strsafe.h>

// Registry helper functions
HRESULT RegisterFilter(REFCLSID clsid, LPCWSTR wszName, LPCWSTR wszDLLName);
HRESULT UnregisterFilter(REFCLSID clsid);
HRESULT SetKeyValue(HKEY hKeyRoot, LPCWSTR pszPath, LPCWSTR pszSubkey, LPCWSTR pszValue);

STDAPI DllRegisterServer(void)
{
    HRESULT hr = S_OK;
    
    // Get the module file name
    WCHAR wszModule[MAX_PATH];
    DWORD dwResult = GetModuleFileNameW(GetModuleHandle(NULL), wszModule, MAX_PATH);
    if (dwResult == 0) {
        return HRESULT_FROM_WIN32(GetLastError());
    }
    
    // Register the filter
    hr = RegisterFilter(CLSID_MySubstituteVirtualCamera, 
                       L"MySubstitute Virtual Camera", 
                       wszModule);
    
    return hr;
}

STDAPI DllUnregisterServer(void)
{
    return UnregisterFilter(CLSID_MySubstituteVirtualCamera);
}

HRESULT RegisterFilter(REFCLSID clsid, LPCWSTR wszName, LPCWSTR wszDLLName)
{
    HRESULT hr = S_OK;
    WCHAR wszCLSID[CHARS_IN_GUID];
    WCHAR wszSubkey[MAX_PATH];
    
    // Convert CLSID to string
    hr = StringFromGUID2(clsid, wszCLSID, CHARS_IN_GUID);
    if (FAILED(hr)) {
        return hr;
    }
    
    // Register CLSID
    StringCchPrintfW(wszSubkey, MAX_PATH, L"CLSID\\%s", wszCLSID);
    hr = SetKeyValue(HKEY_CLASSES_ROOT, wszSubkey, NULL, wszName);
    if (FAILED(hr)) {
        return hr;
    }
    
    // Register InprocServer32
    StringCchPrintfW(wszSubkey, MAX_PATH, L"CLSID\\%s\\InprocServer32", wszCLSID);
    hr = SetKeyValue(HKEY_CLASSES_ROOT, wszSubkey, NULL, wszDLLName);
    if (FAILED(hr)) {
        return hr;
    }
    
    hr = SetKeyValue(HKEY_CLASSES_ROOT, wszSubkey, L"ThreadingModel", L"Both");
    if (FAILED(hr)) {
        return hr;
    }
    
    // Register as DirectShow filter
    StringCchPrintfW(wszSubkey, MAX_PATH, 
        L"CLSID\\{083863F1-70DE-11D0-BD40-00A0C911CE86}\\Instance\\%s", wszCLSID);
    hr = SetKeyValue(HKEY_CLASSES_ROOT, wszSubkey, L"CLSID", wszCLSID);
    if (FAILED(hr)) {
        return hr;
    }
    
    hr = SetKeyValue(HKEY_CLASSES_ROOT, wszSubkey, L"FriendlyName", wszName);
    if (FAILED(hr)) {
        return hr;
    }
    
    return hr;
}

HRESULT UnregisterFilter(REFCLSID clsid)
{
    HRESULT hr = S_OK;
    WCHAR wszCLSID[CHARS_IN_GUID];
    WCHAR wszSubkey[MAX_PATH];
    
    // Convert CLSID to string
    hr = StringFromGUID2(clsid, wszCLSID, CHARS_IN_GUID);
    if (FAILED(hr)) {
        return hr;
    }
    
    // Remove from DirectShow filters
    StringCchPrintfW(wszSubkey, MAX_PATH, 
        L"CLSID\\{083863F1-70DE-11D0-BD40-00A0C911CE86}\\Instance\\%s", wszCLSID);
    RegDeleteKeyW(HKEY_CLASSES_ROOT, wszSubkey);
    
    // Remove CLSID entry
    StringCchPrintfW(wszSubkey, MAX_PATH, L"CLSID\\%s", wszCLSID);
    RegDeleteTreeW(HKEY_CLASSES_ROOT, wszSubkey);
    
    return S_OK;
}

HRESULT SetKeyValue(HKEY hKeyRoot, LPCWSTR pszPath, LPCWSTR pszSubkey, LPCWSTR pszValue)
{
    HKEY hKey;
    WCHAR szKeyBuf[MAX_PATH];
    
    if (pszSubkey != NULL) {
        StringCchPrintfW(szKeyBuf, MAX_PATH, L"%s\\%s", pszPath, pszSubkey);
    } else {
        StringCchCopyW(szKeyBuf, MAX_PATH, pszPath);
    }
    
    LONG lResult = RegCreateKeyExW(hKeyRoot, szKeyBuf, 0, NULL, 
        REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL);
    
    if (lResult != ERROR_SUCCESS) {
        return HRESULT_FROM_WIN32(lResult);
    }
    
    if (pszValue != NULL) {
        lResult = RegSetValueExW(hKey, NULL, 0, REG_SZ, 
            (LPBYTE)pszValue, (lstrlenW(pszValue) + 1) * sizeof(WCHAR));
    }
    
    RegCloseKey(hKey);
    
    return (lResult == ERROR_SUCCESS) ? S_OK : HRESULT_FROM_WIN32(lResult);
}