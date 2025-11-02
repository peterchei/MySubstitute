#include <windows.h>
#include <dshow.h>
#include <iostream>
#include <comdef.h>
#include <ks.h>
#include <ksmedia.h>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "strmiids.lib")

int main() {
    CoInitialize(NULL);
    
    std::cout << "=== BROWSER COMPATIBILITY TEST ===" << std::endl;
    
    // Find our virtual camera
    ICreateDevEnum* pCreateDevEnum = NULL;
    IEnumMoniker* pEnumMoniker = NULL;
    IMoniker* pMoniker = NULL;
    IMoniker* ourDevice = NULL;
    
    CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER,
        IID_ICreateDevEnum, (void**)&pCreateDevEnum);
    
    pCreateDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEnumMoniker, 0);
    
    ULONG fetched;
    while (pEnumMoniker->Next(1, &pMoniker, &fetched) == S_OK) {
        IPropertyBag* pPropertyBag = NULL;
        if (SUCCEEDED(pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void**)&pPropertyBag))) {
            VARIANT varName;
            VariantInit(&varName);
            if (SUCCEEDED(pPropertyBag->Read(L"FriendlyName", &varName, 0))) {
                if (wcsstr(varName.bstrVal, L"MySubstitute") != NULL) {
                    std::cout << "✓ Found MySubstitute Virtual Camera" << std::endl;
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
        std::cout << "❌ Virtual camera not found" << std::endl;
        return -1;
    }
    
    // Test filter creation
    IBaseFilter* pFilter = NULL;
    HRESULT hr = ourDevice->BindToObject(NULL, NULL, IID_IBaseFilter, (void**)&pFilter);
    if (FAILED(hr)) {
        std::cout << "❌ Filter creation failed" << std::endl;
        return -1;
    }
    std::cout << "✓ Filter created successfully" << std::endl;
    
    // Test IKsPropertySet on filter
    IKsPropertySet* pKsProp = NULL;
    hr = pFilter->QueryInterface(IID_IKsPropertySet, (void**)&pKsProp);
    if (SUCCEEDED(hr)) {
        std::cout << "✓ Filter supports IKsPropertySet" << std::endl;
        
        // Test property query
        DWORD dwSupported = 0;
        hr = pKsProp->QuerySupported(AMPROPSETID_Pin, AMPROPERTY_PIN_CATEGORY, &dwSupported);
        if (SUCCEEDED(hr)) {
            std::cout << "✓ Filter supports PIN_CATEGORY property" << std::endl;
        } else {
            std::cout << "⚠ Filter PIN_CATEGORY query failed: 0x" << std::hex << hr << std::endl;
        }
        
        pKsProp->Release();
    } else {
        std::cout << "❌ Filter does not support IKsPropertySet" << std::endl;
    }
    
    // Test pin interfaces
    IEnumPins* pEnumPins = NULL;
    hr = pFilter->EnumPins(&pEnumPins);
    if (SUCCEEDED(hr)) {
        IPin* pPin = NULL;
        ULONG pinsFetched = 0;
        
        while (pEnumPins->Next(1, &pPin, &pinsFetched) == S_OK) {
            PIN_DIRECTION direction;
            pPin->QueryDirection(&direction);
            
            if (direction == PINDIR_OUTPUT) {
                std::cout << "✓ Found output pin" << std::endl;
                
                // Test IKsPropertySet on pin
                IKsPropertySet* pPinKsProp = NULL;
                hr = pPin->QueryInterface(IID_IKsPropertySet, (void**)&pPinKsProp);
                if (SUCCEEDED(hr)) {
                    std::cout << "✓ Pin supports IKsPropertySet" << std::endl;
                    
                    // Test pin category property
                    GUID category;
                    DWORD dwReturned = 0;
                    hr = pPinKsProp->Get(AMPROPSETID_Pin, AMPROPERTY_PIN_CATEGORY, 
                                       NULL, 0, &category, sizeof(GUID), &dwReturned);
                    if (SUCCEEDED(hr) && category == PIN_CATEGORY_CAPTURE) {
                        std::cout << "✓ Pin correctly identifies as CAPTURE category" << std::endl;
                    } else {
                        std::cout << "⚠ Pin category query failed or wrong category: 0x" << std::hex << hr << std::endl;
                    }
                    
                    pPinKsProp->Release();
                } else {
                    std::cout << "❌ Pin does not support IKsPropertySet" << std::endl;
                }
                
                // Test IAMStreamConfig
                IAMStreamConfig* pStreamConfig = NULL;
                hr = pPin->QueryInterface(IID_IAMStreamConfig, (void**)&pStreamConfig);
                if (SUCCEEDED(hr)) {
                    std::cout << "✓ Pin supports IAMStreamConfig" << std::endl;
                    
                    int iCount = 0, iSize = 0;
                    hr = pStreamConfig->GetNumberOfCapabilities(&iCount, &iSize);
                    if (SUCCEEDED(hr)) {
                        std::cout << "✓ Pin reports " << iCount << " capabilities" << std::endl;
                    } else {
                        std::cout << "⚠ GetNumberOfCapabilities failed: 0x" << std::hex << hr << std::endl;
                    }
                    
                    pStreamConfig->Release();
                } else {
                    std::cout << "❌ Pin does not support IAMStreamConfig" << std::endl;
                }
                
                // Test EnumMediaTypes
                IEnumMediaTypes* pEnumMT = NULL;
                hr = pPin->EnumMediaTypes(&pEnumMT);
                if (SUCCEEDED(hr)) {
                    std::cout << "✓ Pin supports media type enumeration" << std::endl;
                    
                    AM_MEDIA_TYPE* pmt = NULL;
                    ULONG mtFetched = 0;
                    hr = pEnumMT->Next(1, &pmt, &mtFetched);
                    if (SUCCEEDED(hr) && mtFetched > 0) {
                        std::cout << "✓ Pin provides media types" << std::endl;
                        if (pmt->pbFormat) CoTaskMemFree(pmt->pbFormat);
                        CoTaskMemFree(pmt);
                    } else {
                        std::cout << "⚠ No media types available" << std::endl;
                    }
                    
                    pEnumMT->Release();
                } else {
                    std::cout << "❌ Pin EnumMediaTypes failed: 0x" << std::hex << hr << std::endl;
                }
            }
            
            pPin->Release();
        }
        
        pEnumPins->Release();
    }
    
    std::cout << "=== BROWSER COMPATIBILITY TEST COMPLETE ===" << std::endl;
    
    // Cleanup
    pFilter->Release();
    ourDevice->Release();
    pEnumMoniker->Release();
    pCreateDevEnum->Release();
    CoUninitialize();
    
    return 0;
}