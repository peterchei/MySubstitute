#include <iostream>
#include <memory>
#include <windows.h>

#include "capture/camera_capture.h"
#include "ai/ai_processor.h"
#include "virtual_camera/virtual_camera_filter.h"
#include "service/background_service.h"

int main() {
    std::cout << "MySubstitute Virtual Camera - Test Application" << std::endl;
    
    // Initialize COM for DirectShow
    HRESULT hr = CoInitialize(nullptr);
    if (FAILED(hr)) {
        std::cerr << "Failed to initialize COM" << std::endl;
        return -1;
    }
    
    try {
        // Test camera enumeration
        std::cout << "Initializing camera capture..." << std::endl;
        auto camera = CameraCapture::Create();
        
        if (!camera->Initialize()) {
            std::cerr << "Failed to initialize camera capture" << std::endl;
            return -1;
        }
        
        auto cameras = camera->GetAvailableCameras();
        std::cout << "Found " << cameras.size() << " cameras:" << std::endl;
        
        for (size_t i = 0; i < cameras.size(); ++i) {
            std::cout << "  " << i << ": " << cameras[i].name << std::endl;
        }
        
        // Test AI processor
        std::cout << "Testing AI processor..." << std::endl;
        auto processor = std::make_unique<PassthroughProcessor>();
        if (!processor->Initialize()) {
            std::cerr << "Failed to initialize AI processor" << std::endl;
            return -1;
        }
        
        std::cout << "All components initialized successfully!" << std::endl;
        std::cout << "Press Enter to exit..." << std::endl;
        std::cin.get();
        
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
    
    CoUninitialize();
    return 0;
}