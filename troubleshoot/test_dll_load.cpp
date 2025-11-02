#include <windows.h>
#include <iostream>

int main()
{
    std::wcout << L"🧪 Testing DLL Loading" << std::endl;
    std::wcout << L"=====================" << std::endl;

    // Try to load our DLL
    HMODULE hDLL = LoadLibraryW(L"C:\\Users\\peter\\git\\MySubstitute\\build\\bin\\Release\\MySubstituteVirtualCamera.dll");
    
    if (hDLL) {
        std::wcout << L"✅ DLL loaded successfully!" << std::endl;
        
        // Check if DLL exports exist
        auto pDllGetClassObject = GetProcAddress(hDLL, "DllGetClassObject");
        auto pDllCanUnloadNow = GetProcAddress(hDLL, "DllCanUnloadNow");
        auto pDllRegisterServer = GetProcAddress(hDLL, "DllRegisterServer");
        auto pDllUnregisterServer = GetProcAddress(hDLL, "DllUnregisterServer");
        
        std::wcout << L"DllGetClassObject: " << (pDllGetClassObject ? L"✅ Found" : L"❌ Missing") << std::endl;
        std::wcout << L"DllCanUnloadNow: " << (pDllCanUnloadNow ? L"✅ Found" : L"❌ Missing") << std::endl;
        std::wcout << L"DllRegisterServer: " << (pDllRegisterServer ? L"✅ Found" : L"❌ Missing") << std::endl;
        std::wcout << L"DllUnregisterServer: " << (pDllUnregisterServer ? L"✅ Found" : L"❌ Missing") << std::endl;
        
        FreeLibrary(hDLL);
    } else {
        DWORD error = GetLastError();
        std::wcout << L"❌ Failed to load DLL. Error: " << error << std::endl;
    }
    
    std::wcout << L"\nPress Enter to continue..." << std::endl;
    std::wcin.get();
    
    return 0;
}