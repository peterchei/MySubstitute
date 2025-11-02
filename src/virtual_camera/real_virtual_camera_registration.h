#pragma once
#include <windows.h>
#include <string>

/**
 * Real Virtual Camera Registration
 * Actually registers a DirectShow source filter that appears as a camera
 */
class RealVirtualCameraRegistration {
public:
    // Register our DirectShow filter as a system video input device
    static bool RegisterVirtualCameraFilter();
    
    // Unregister the virtual camera filter
    static bool UnregisterVirtualCameraFilter();
    
    // Check if our virtual camera is registered
    static bool IsVirtualCameraRegistered();
    
    // Test if the virtual camera appears in device enumeration
    static bool TestVirtualCameraVisibility();
    
    // Register the filter and all required COM components
    static bool RegisterCompleteVirtualCamera();
    
    // Show detailed status of registration
    static void ShowRegistrationStatus();

private:
    // Register the DirectShow filter in the system registry
    static bool RegisterDirectShowFilter();
    
    // Register as a video input device category
    static bool RegisterAsVideoInputDevice();
    
    // Create all necessary registry entries
    static bool CreateRegistryEntries();
    
    // Register the COM server (DLL)
    static bool RegisterCOMServer();
    
    // Verify the registration worked
    static bool VerifyRegistration();
    
    static const wchar_t* GetFilterCLSID();
    static const wchar_t* GetFilterName();
};