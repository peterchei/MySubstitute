#include <windows.h>
#include <dshow.h>
#include <iostream>
#include <comdef.h>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "strmiids.lib")

int main() {
    CoInitialize(NULL);
    
    ICreateDevEnum* pCreateDevEnum = NULL;
    IEnumMoniker* pEnumMoniker = NULL;
    IMoniker* pMoniker = NULL;
    
    HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER,
        IID_ICreateDevEnum, (void**)&pCreateDevEnum);
    
    if (FAILED(hr)) {
        std::cout << "Failed to create device enumerator: " << std::hex << hr << std::endl;
        return -1;
    }
    
    hr = pCreateDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEnumMoniker, 0);
    if (hr == S_FALSE) {
        std::cout << "No video devices found" << std::endl;
        return -1;
    }
    
    std::cout << "Available video devices:" << std::endl;
    
    ULONG fetched;
    int deviceCount = 0;
    while (pEnumMoniker->Next(1, &pMoniker, &fetched) == S_OK) {
        IPropertyBag* pPropertyBag;
        hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void**)&pPropertyBag);
        
        if (SUCCEEDED(hr)) {
            VARIANT varName;
            VariantInit(&varName);
            hr = pPropertyBag->Read(L"Description", &varName, 0);
            if (FAILED(hr)) {
                hr = pPropertyBag->Read(L"FriendlyName", &varName, 0);
            }
            
            if (SUCCEEDED(hr)) {
                _bstr_t bstrName(varName.bstrVal);
                std::cout << deviceCount << ": " << (char*)bstrName << std::endl;
                
                // Check if this is our virtual camera
                if (wcsstr(varName.bstrVal, L"MySubstitute") != NULL) {
                    std::cout << "  *** FOUND OUR VIRTUAL CAMERA! ***" << std::endl;
                }
            }
            
            VariantClear(&varName);
            pPropertyBag->Release();
        }
        
        pMoniker->Release();
        deviceCount++;
    }
    
    std::cout << "Total devices found: " << deviceCount << std::endl;
    
    pEnumMoniker->Release();
    pCreateDevEnum->Release();
    CoUninitialize();
    
    return 0;
}