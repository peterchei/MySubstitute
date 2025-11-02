#include <windows.h>
#include <dshow.h>
#include <initguid.h>
#include <iostream>
#include <comutil.h>

// MySubstitute Virtual Camera CLSID
DEFINE_GUID(CLSID_MySubstituteVirtualCamera,
    0xb3f3a1c4, 0x8f9e, 0x4a2d, 0x9b, 0x5c, 0x7e, 0x6f, 0x8d, 0x4c, 0x9a, 0x3b);

void TestDirectShowDevice()
{
    std::wcout << L"🧪 Testing DirectShow Virtual Camera Creation" << std::endl;
    std::wcout << L"=============================================" << std::endl;

    HRESULT hr = CoInitialize(nullptr);
    if (FAILED(hr)) {
        std::wcout << L"❌ Failed to initialize COM: 0x" << std::hex << hr << std::endl;
        return;
    }

    // Test 1: Try to create our filter directly via COM
    std::wcout << L"\n📊 Test 1: Direct COM creation..." << std::endl;
    IBaseFilter* pFilter = nullptr;
    hr = CoCreateInstance(
        CLSID_MySubstituteVirtualCamera,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_IBaseFilter,
        (void**)&pFilter
    );

    if (SUCCEEDED(hr) && pFilter) {
        std::wcout << L"✅ Filter created successfully!" << std::endl;
        
        // Test filter capabilities
        FILTER_INFO filterInfo = {};
        hr = pFilter->QueryFilterInfo(&filterInfo);
        if (SUCCEEDED(hr)) {
            std::wcout << L"📊 Filter Name: " << filterInfo.achName << std::endl;
            if (filterInfo.pGraph) filterInfo.pGraph->Release();
        }

        // Test pin enumeration
        IEnumPins* pEnumPins = nullptr;
        hr = pFilter->EnumPins(&pEnumPins);
        if (SUCCEEDED(hr)) {
            IPin* pPin = nullptr;
            ULONG pinCount = 0;
            while (pEnumPins->Next(1, &pPin, nullptr) == S_OK) {
                pinCount++;
                PIN_INFO pinInfo = {};
                if (SUCCEEDED(pPin->QueryPinInfo(&pinInfo))) {
                    std::wcout << L"📍 Pin " << pinCount << L": " << pinInfo.achName << std::endl;
                    if (pinInfo.pFilter) pinInfo.pFilter->Release();
                }
                pPin->Release();
            }
            pEnumPins->Release();
            std::wcout << L"📊 Total pins: " << pinCount << std::endl;
        }

        pFilter->Release();
    } else {
        std::wcout << L"❌ Failed to create filter: HRESULT 0x" << std::hex << hr << std::endl;
        
        // Decode error
        switch (hr) {
            case REGDB_E_CLASSNOTREG:
                std::wcout << L"❌ Class not registered" << std::endl;
                break;
            case CLASS_E_NOAGGREGATION:
                std::wcout << L"❌ No aggregation allowed" << std::endl;
                break;
            case E_NOINTERFACE:
                std::wcout << L"❌ Interface not supported" << std::endl;
                break;
            case CO_E_DLLNOTFOUND:
                std::wcout << L"❌ DLL not found" << std::endl;
                break;
            case CO_E_ERRORINDLL:
                std::wcout << L"❌ Error in DLL" << std::endl;
                break;
            default:
                std::wcout << L"❌ Unknown error: 0x" << std::hex << hr << std::endl;
        }
    }

    // Test 2: Try DirectShow device enumeration 
    std::wcout << L"\n📊 Test 2: DirectShow device enumeration..." << std::endl;
    
    ICreateDevEnum* pDevEnum = nullptr;
    hr = CoCreateInstance(CLSID_SystemDeviceEnum, nullptr, CLSCTX_INPROC_SERVER,
                         IID_ICreateDevEnum, (void**)&pDevEnum);
    
    if (SUCCEEDED(hr)) {
        IEnumMoniker* pEnumMoniker = nullptr;
        hr = pDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEnumMoniker, 0);
        
        if (hr == S_OK) {
            IMoniker* pMoniker = nullptr;
            bool foundOurDevice = false;
            
            while (pEnumMoniker->Next(1, &pMoniker, nullptr) == S_OK) {
                IPropertyBag* pPropBag = nullptr;
                hr = pMoniker->BindToStorage(nullptr, nullptr, IID_IPropertyBag, (void**)&pPropBag);
                
                if (SUCCEEDED(hr)) {
                    VARIANT varName;
                    VariantInit(&varName);
                    hr = pPropBag->Read(L"FriendlyName", &varName, nullptr);
                    
                    if (SUCCEEDED(hr)) {
                        std::wcout << L"🎥 Found device: " << varName.bstrVal << std::endl;
                        
                        if (wcsstr(varName.bstrVal, L"MySubstitute")) {
                            std::wcout << L"✅ Found our virtual camera!" << std::endl;
                            foundOurDevice = true;
                            
                            // Try to bind to our device
                            IBaseFilter* pDeviceFilter = nullptr;
                            hr = pMoniker->BindToObject(nullptr, nullptr, IID_IBaseFilter, (void**)&pDeviceFilter);
                            
                            if (SUCCEEDED(hr)) {
                                std::wcout << L"✅ Successfully bound to device!" << std::endl;
                                pDeviceFilter->Release();
                            } else {
                                std::wcout << L"❌ Failed to bind to device: 0x" << std::hex << hr << std::endl;
                            }
                        }
                    }
                    VariantClear(&varName);
                    pPropBag->Release();
                }
                pMoniker->Release();
            }
            
            if (!foundOurDevice) {
                std::wcout << L"❌ Our virtual camera not found in device enumeration!" << std::endl;
            }
            
            pEnumMoniker->Release();
        }
        pDevEnum->Release();
    }

    CoUninitialize();
}

int main()
{
    TestDirectShowDevice();
    
    std::wcout << L"\nPress Enter to exit..." << std::endl;
    std::wcin.get();
    
    return 0;
}