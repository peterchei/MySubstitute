#include <windows.h>
#include <dshow.h>
#include <iostream>
#include <comdef.h>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "strmiids.lib")

int main() {
    std::cout << "=== MINIMAL CRASH TEST ===" << std::endl;
    
    HRESULT hr = CoInitialize(NULL);
    if (FAILED(hr)) {
        std::cout << "FAILED: CoInitialize" << std::endl;
        return -1;
    }
    
    std::cout << "Step 1: CoInitialize - OK" << std::endl;
    
    ICreateDevEnum* pCreateDevEnum = NULL;
    hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER,
        IID_ICreateDevEnum, (void**)&pCreateDevEnum);
    
    if (FAILED(hr)) {
        std::cout << "FAILED: CoCreateInstance" << std::endl;
        CoUninitialize();
        return -1;
    }
    
    std::cout << "Step 2: CoCreateInstance - OK" << std::endl;
    
    IEnumMoniker* pEnumMoniker = NULL;
    hr = pCreateDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEnumMoniker, 0);
    if (FAILED(hr)) {
        std::cout << "FAILED: CreateClassEnumerator" << std::endl;
        pCreateDevEnum->Release();
        CoUninitialize();
        return -1;
    }
    
    std::cout << "Step 3: CreateClassEnumerator - OK" << std::endl;
    
    IMoniker* pMoniker = NULL;
    ULONG fetched;
    IMoniker* ourDevice = NULL;
    
    while (pEnumMoniker->Next(1, &pMoniker, &fetched) == S_OK) {
        IPropertyBag* pPropertyBag = NULL;
        hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void**)&pPropertyBag);
        
        if (SUCCEEDED(hr)) {
            VARIANT varName;
            VariantInit(&varName);
            hr = pPropertyBag->Read(L"FriendlyName", &varName, 0);
            
            if (SUCCEEDED(hr)) {
                if (wcsstr(varName.bstrVal, L"MySubstitute") != NULL) {
                    std::cout << "Step 4: Found MySubstitute Virtual Camera" << std::endl;
                    ourDevice = pMoniker;
                    ourDevice->AddRef();
                    VariantClear(&varName);
                    pPropertyBag->Release();
                    break;
                }
            }
            
            VariantClear(&varName);
            pPropertyBag->Release();
        }
        
        pMoniker->Release();
    }
    
    if (!ourDevice) {
        std::cout << "FAILED: Device not found" << std::endl;
        pEnumMoniker->Release();
        pCreateDevEnum->Release();
        CoUninitialize();
        return -1;
    }
    
    std::cout << "Step 5: About to create filter instance..." << std::endl;
    std::cout << "THIS IS WHERE THE CRASH LIKELY HAPPENS" << std::endl;
    
    IBaseFilter* pFilter = NULL;
    hr = ourDevice->BindToObject(NULL, NULL, IID_IBaseFilter, (void**)&pFilter);
    
    if (FAILED(hr)) {
        std::cout << "FAILED: BindToObject failed with HR: 0x" << std::hex << hr << std::endl;
        _com_error err(hr);
        std::wcout << L"Error: " << err.ErrorMessage() << std::endl;
    } else {
        std::cout << "Step 6: Filter created successfully!" << std::endl;
        
        if (pFilter) {
            pFilter->Release();
        }
    }
    
    // Cleanup
    ourDevice->Release();
    pEnumMoniker->Release();
    pCreateDevEnum->Release();
    CoUninitialize();
    
    std::cout << "=== TEST COMPLETE ===" << std::endl;
    return 0;
}