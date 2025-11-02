#include <windows.h>
#include <dshow.h>
#include <iostream>
#include <comdef.h>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "strmiids.lib")

int main() {
    CoInitialize(NULL);
    
    std::cout << "Testing MySubstitute Virtual Camera functionality..." << std::endl;
    
    // 1. Check if device exists
    ICreateDevEnum* pCreateDevEnum = NULL;
    IEnumMoniker* pEnumMoniker = NULL;
    IMoniker* pMoniker = NULL;
    
    HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER,
        IID_ICreateDevEnum, (void**)&pCreateDevEnum);
    
    if (FAILED(hr)) {
        std::cout << "FAILED: Cannot create device enumerator" << std::endl;
        return -1;
    }
    
    hr = pCreateDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEnumMoniker, 0);
    if (hr == S_FALSE) {
        std::cout << "FAILED: No video devices found" << std::endl;
        return -1;
    }
    
    IMoniker* ourDevice = NULL;
    ULONG fetched;
    bool found = false;
    
    while (pEnumMoniker->Next(1, &pMoniker, &fetched) == S_OK) {
        IPropertyBag* pPropertyBag;
        hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void**)&pPropertyBag);
        
        if (SUCCEEDED(hr)) {
            VARIANT varName;
            VariantInit(&varName);
            hr = pPropertyBag->Read(L"FriendlyName", &varName, 0);
            
            if (SUCCEEDED(hr)) {
                if (wcsstr(varName.bstrVal, L"MySubstitute") != NULL) {
                    std::cout << "✓ Found MySubstitute Virtual Camera" << std::endl;
                    ourDevice = pMoniker;
                    ourDevice->AddRef();
                    found = true;
                }
            }
            
            VariantClear(&varName);
            pPropertyBag->Release();
        }
        
        pMoniker->Release();
    }
    
    if (!found) {
        std::cout << "FAILED: MySubstitute Virtual Camera not found" << std::endl;
        return -1;
    }
    
    // 2. Try to create the filter
    IBaseFilter* pFilter = NULL;
    hr = ourDevice->BindToObject(NULL, NULL, IID_IBaseFilter, (void**)&pFilter);
    
    if (FAILED(hr)) {
        std::cout << "FAILED: Cannot create filter instance. HRESULT: 0x" << std::hex << hr << std::endl;
        _com_error err(hr);
        std::wcout << L"Error: " << err.ErrorMessage() << std::endl;
        return -1;
    }
    
    std::cout << "✓ Successfully created filter instance" << std::endl;
    
    // 3. Check filter interfaces
    IAMStreamConfig* pStreamConfig = NULL;
    hr = pFilter->QueryInterface(IID_IAMStreamConfig, (void**)&pStreamConfig);
    if (SUCCEEDED(hr)) {
        std::cout << "✓ IAMStreamConfig interface available" << std::endl;
        pStreamConfig->Release();
    } else {
        std::cout << "⚠ IAMStreamConfig interface not available" << std::endl;
    }
    
    // 4. Try to get pins
    IEnumPins* pEnumPins = NULL;
    hr = pFilter->EnumPins(&pEnumPins);
    if (SUCCEEDED(hr)) {
        IPin* pPin = NULL;
        ULONG pinsFetched = 0;
        int pinCount = 0;
        
        while (pEnumPins->Next(1, &pPin, &pinsFetched) == S_OK) {
            PIN_DIRECTION direction;
            pPin->QueryDirection(&direction);
            
            if (direction == PINDIR_OUTPUT) {
                std::cout << "✓ Found output pin " << pinCount << std::endl;
            }
            
            pPin->Release();
            pinCount++;
        }
        
        std::cout << "✓ Total pins: " << pinCount << std::endl;
        pEnumPins->Release();
    }
    
    std::cout << "✓ Virtual camera appears to be working correctly!" << std::endl;
    
    // Cleanup
    pFilter->Release();
    ourDevice->Release();
    pEnumMoniker->Release();
    pCreateDevEnum->Release();
    CoUninitialize();
    
    return 0;
}