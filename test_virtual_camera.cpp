#include <iostream>
#include <windows.h>
#include "src/virtual_camera/simple_registry_virtual_camera.h"

int main() {
    std::wcout << L"🧪 Testing SimpleRegistryVirtualCamera..." << std::endl;
    std::wcout << L"=========================================" << std::endl;
    
    // Initialize COM
    CoInitialize(nullptr);
    
    // Show initial status
    std::wcout << L"\n📊 Initial Status:" << std::endl;
    SimpleRegistryVirtualCamera::ShowDetailedStatus();
    
    // Try to register
    std::wcout << L"\n🔄 Attempting registration..." << std::endl;
    if (SimpleRegistryVirtualCamera::RegisterWithAdminCheck()) {
        std::wcout << L"✅ Registration successful!" << std::endl;
        
        // Show final status
        std::wcout << L"\n📊 Final Status:" << std::endl;
        SimpleRegistryVirtualCamera::ShowDetailedStatus();
    } else {
        std::wcout << L"❌ Registration failed!" << std::endl;
    }
    
    std::wcout << L"\nPress Enter to continue..." << std::endl;
    std::cin.get();
    
    // Cleanup COM
    CoUninitialize();
    
    return 0;
}