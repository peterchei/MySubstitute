#pragma once
#include <windows.h>
#include <string>

/**
 * OBS Virtual Camera Integration
 * Uses existing OBS virtual camera if available, or guides user to install it
 */
class OBSVirtualCameraHelper {
public:
    // Check if OBS Virtual Camera is installed
    static bool IsOBSVirtualCameraInstalled();
    
    // Check if OBS is currently running
    static bool IsOBSRunning();
    
    // Guide user to install OBS Studio
    static void ShowOBSInstallationGuide();
    
    // Try to start OBS virtual camera programmatically
    static bool StartOBSVirtualCamera();
    
    // Show instructions for manual OBS virtual camera setup
    static void ShowOBSSetupInstructions();
    
    // Check if we can integrate with OBS
    static bool CanIntegrateWithOBS();
    
private:
    static bool CheckOBSRegistry();
    static bool CheckOBSProcess();
    static std::wstring GetOBSInstallPath();
};