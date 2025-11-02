#include <windows.h>
#include <dshow.h>
#include <iostream>
#include <comdef.h>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "strmiids.lib")

int main() {
    HRESULT hr = CoInitialize(NULL);
    if (FAILED(hr)) {
        std::cout << "FAILED: CoInitialize failed" << std::endl;
        return -1;
    }
    
    std::cout << "Testing MySubstitute Virtual Camera functionality..." << std::endl;
    
    ICreateDevEnum* pCreateDevEnum = NULL;
    IEnumMoniker* pEnumMoniker = NULL;
    IMoniker* pMoniker = NULL;
    IMoniker* ourDevice = NULL;
    IBaseFilter* pFilter = NULL;
    
    __try {
        // 1. Check if device exists
        hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER,
            IID_ICreateDevEnum, (void**)&pCreateDevEnum);
        
        if (FAILED(hr)) {
            std::cout << "FAILED: Cannot create device enumerator. HR: 0x" << std::hex << hr << std::endl;
            __leave;
        }
        
        hr = pCreateDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEnumMoniker, 0);
        if (hr == S_FALSE) {
            std::cout << "FAILED: No video devices found" << std::endl;
            __leave;
        }
        
        if (FAILED(hr)) {
            std::cout << "FAILED: CreateClassEnumerator failed. HR: 0x" << std::hex << hr << std::endl;
            __leave;
        }
        
        ULONG fetched;
        bool found = false;
        
        while (pEnumMoniker->Next(1, &pMoniker, &fetched) == S_OK) {
            IPropertyBag* pPropertyBag = NULL;
            hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void**)&pPropertyBag);
            
            if (SUCCEEDED(hr) && pPropertyBag) {
                VARIANT varName;
                VariantInit(&varName);
                hr = pPropertyBag->Read(L"FriendlyName", &varName, 0);
                
                if (SUCCEEDED(hr) && varName.vt == VT_BSTR && varName.bstrVal) {
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
            pMoniker = NULL;
            
            if (found) break;
        }
        
        if (!found) {
            std::cout << "FAILED: MySubstitute Virtual Camera not found" << std::endl;
            __leave;
        }
        
        // 2. Try to create the filter - THIS IS WHERE THE CRASH LIKELY HAPPENS
        std::cout << "Attempting to create filter instance..." << std::endl;
        hr = ourDevice->BindToObject(NULL, NULL, IID_IBaseFilter, (void**)&pFilter);
        
        if (FAILED(hr)) {
            std::cout << "FAILED: Cannot create filter instance. HRESULT: 0x" << std::hex << hr << std::endl;
            _com_error err(hr);
            std::wcout << L"Error: " << err.ErrorMessage() << std::endl;
            __leave;
        }
        
        if (!pFilter) {
            std::cout << "FAILED: Filter pointer is NULL" << std::endl;
            __leave;
        }
        
        std::cout << "✓ Successfully created filter instance" << std::endl;
        
        // 3. Check filter interfaces
        IAMStreamConfig* pStreamConfig = NULL;
        hr = pFilter->QueryInterface(IID_IAMStreamConfig, (void**)&pStreamConfig);
        if (SUCCEEDED(hr) && pStreamConfig) {
            std::cout << "✓ IAMStreamConfig interface available" << std::endl;
            pStreamConfig->Release();
        } else {
            std::cout << "⚠ IAMStreamConfig interface not available. HR: 0x" << std::hex << hr << std::endl;
        }
        
        // 4. Try to get pins
        IEnumPins* pEnumPins = NULL;
        hr = pFilter->EnumPins(&pEnumPins);
        if (SUCCEEDED(hr) && pEnumPins) {
            IPin* pPin = NULL;
            ULONG pinsFetched = 0;
            int pinCount = 0;
            
            while (pEnumPins->Next(1, &pPin, &pinsFetched) == S_OK && pPin) {
                PIN_DIRECTION direction;
                hr = pPin->QueryDirection(&direction);
                if (SUCCEEDED(hr)) {
                    if (direction == PINDIR_OUTPUT) {
                        std::cout << "✓ Found output pin " << pinCount << std::endl;
                        
                        // Test EnumMediaTypes - this might be where it crashes
                        IEnumMediaTypes* pEnumMT = NULL;
                        hr = pPin->EnumMediaTypes(&pEnumMT);
                        if (SUCCEEDED(hr) && pEnumMT) {
                            std::cout << "✓ Pin supports media type enumeration" << std::endl;
                            pEnumMT->Release();
                        } else {
                            std::cout << "⚠ Pin media type enumeration failed. HR: 0x" << std::hex << hr << std::endl;
                        }
                    }
                }
                
                pPin->Release();
                pinCount++;
            }
            
            std::cout << "✓ Total pins: " << pinCount << std::endl;
            pEnumPins->Release();
        } else {
            std::cout << "⚠ EnumPins failed. HR: 0x" << std::hex << hr << std::endl;
        }
        
        std::cout << "✓ Virtual camera appears to be working correctly!" << std::endl;
    }
    __finally {
        // Cleanup
        if (pFilter) pFilter->Release();
        if (ourDevice) ourDevice->Release();
        if (pEnumMoniker) pEnumMoniker->Release();
        if (pCreateDevEnum) pCreateDevEnum->Release();
        CoUninitialize();
    }
    
    return 0;
}