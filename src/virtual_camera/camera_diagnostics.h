#pragma once
#include <windows.h>
#include <string>
#include <vector>

/**
 * Simple camera diagnostics utility
 */
class CameraDiagnostics {
public:
    struct CameraDevice {
        std::wstring name;
        std::wstring devicePath;
        bool isAvailable;
    };
    
    // Test camera visibility and access
    static bool TestCameraSystem();
    static std::vector<CameraDevice> ListAllCameras();
    static bool TestCameraAccess(const std::wstring& devicePath);
    
    // Show results in a message box
    static void ShowDiagnosticsResults();
};