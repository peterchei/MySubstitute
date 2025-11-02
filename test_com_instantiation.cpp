#include <windows.h>
#include <iostream>
#include <objbase.h>
#include <dshow.h>
#include <initguid.h>

// MySubstitute Virtual Camera CLSID
// {B3F3A1C4-8F9E-4A2D-9B5C-7E6F8D4C9A3B}
DEFINE_GUID(CLSID_MySubstituteVirtualCamera,
    0xb3f3a1c4, 0x8f9e, 0x4a2d, 0x9b, 0x5c, 0x7e, 0x6f, 0x8d, 0x4c, 0x9a, 0x3b);

int main()
{
    std::wcout << L"🧪 Testing DirectShow Virtual Camera COM Instantiation" << std::endl;
    std::wcout << L"=======================================================" << std::endl;
    
    HRESULT hr = CoInitialize(nullptr);
    if (FAILED(hr)) {
        std::wcout << L"❌ Failed to initialize COM: 0x" << std::hex << hr << std::endl;
        return -1;
    }
    
    std::wcout << L"✅ COM initialized successfully" << std::endl;
    
    // Try to create our virtual camera filter
    IBaseFilter* pFilter = nullptr;
    hr = CoCreateInstance(
        CLSID_MySubstituteVirtualCamera,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_IBaseFilter,
        (void**)&pFilter
    );
    
    if (SUCCEEDED(hr)) {
        std::wcout << L"✅ Successfully created MySubstitute Virtual Camera filter!" << std::endl;
        
        // Try to get filter info
        FILTER_INFO filterInfo;
        ZeroMemory(&filterInfo, sizeof(filterInfo));
        
        hr = pFilter->QueryFilterInfo(&filterInfo);
        if (SUCCEEDED(hr)) {
            std::wcout << L"📊 Filter Name: " << (filterInfo.achName ? filterInfo.achName : L"(null)") << std::endl;
            if (filterInfo.pGraph) {
                filterInfo.pGraph->Release();
            }
        }
        
        // Try to enumerate pins
        IEnumPins* pEnumPins = nullptr;
        hr = pFilter->EnumPins(&pEnumPins);
        if (SUCCEEDED(hr)) {
            ULONG pinCount = 0;
            IPin* pPin = nullptr;
            while (pEnumPins->Next(1, &pPin, nullptr) == S_OK) {
                pinCount++;
                
                PIN_INFO pinInfo;
                ZeroMemory(&pinInfo, sizeof(pinInfo));
                if (SUCCEEDED(pPin->QueryPinInfo(&pinInfo))) {
                    std::wcout << L"📍 Pin " << pinCount << L": " << pinInfo.achName 
                              << L" (Direction: " << (pinInfo.dir == PINDIR_OUTPUT ? L"Output" : L"Input") << L")" << std::endl;
                    if (pinInfo.pFilter) {
                        pinInfo.pFilter->Release();
                    }
                }
                
                pPin->Release();
            }
            std::wcout << L"📊 Total pins found: " << pinCount << std::endl;
            pEnumPins->Release();
        }
        
        pFilter->Release();
    } else {
        std::wcout << L"❌ Failed to create MySubstitute Virtual Camera filter!" << std::endl;
        std::wcout << L"❌ HRESULT: 0x" << std::hex << hr << std::endl;
        
        // Decode common error codes
        switch (hr) {
            case REGDB_E_CLASSNOTREG:
                std::wcout << L"❌ Error: Class not registered (REGDB_E_CLASSNOTREG)" << std::endl;
                break;
            case CLASS_E_NOAGGREGATION:
                std::wcout << L"❌ Error: No aggregation (CLASS_E_NOAGGREGATION)" << std::endl;
                break;
            case E_NOINTERFACE:
                std::wcout << L"❌ Error: No interface (E_NOINTERFACE)" << std::endl;
                break;
            case E_OUTOFMEMORY:
                std::wcout << L"❌ Error: Out of memory (E_OUTOFMEMORY)" << std::endl;
                break;
            case E_INVALIDARG:
                std::wcout << L"❌ Error: Invalid argument (E_INVALIDARG)" << std::endl;
                break;
            case CO_E_DLLNOTFOUND:
                std::wcout << L"❌ Error: DLL not found (CO_E_DLLNOTFOUND)" << std::endl;
                break;
            case CO_E_APPNOTFOUND:
                std::wcout << L"❌ Error: Application not found (CO_E_APPNOTFOUND)" << std::endl;
                break;
            default:
                std::wcout << L"❌ Error: Unknown error code" << std::endl;
                break;
        }
    }
    
    CoUninitialize();
    
    std::wcout << L"\nPress Enter to exit..." << std::endl;
    std::cin.get();
    
    return 0;
}