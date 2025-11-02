#pragma once
#include <windows.h>

/**
 * Simple Registry Virtual Camera
 * Creates registry entries that make a virtual camera appear in device lists
 * This is a minimal approach that should work without complex DirectShow implementation
 */
class SimpleRegistryVirtualCamera {
public:
    // Register a virtual camera device in Windows registry
    static bool CreateVirtualCameraDevice();
    
    // Remove the virtual camera device registration
    static bool RemoveVirtualCameraDevice();
    
    // Check if the virtual camera is registered
    static bool IsVirtualCameraRegistered();
    
    // Test if the virtual camera appears in device enumeration
    static bool TestDeviceVisibility();
    
    // Show comprehensive status and troubleshooting
    static void ShowDetailedStatus();
    
    // Register with administrator privileges check
    static bool RegisterWithAdminCheck();

private:
    // Check if current user has administrator privileges
    static bool CheckIfUserIsAdmin();
    // Create all necessary registry entries for a virtual camera
    static bool CreateDeviceRegistryEntries();
    
    // Create DirectShow filter registration
    static bool CreateDirectShowEntries();
    
    // Create device interface entries
    static bool CreateDeviceInterfaceEntries();
    
    // Verify all registry entries exist
    static bool VerifyAllEntries();
    
    static const wchar_t* GetDeviceGUID();
    static const wchar_t* GetDeviceName();
    static const wchar_t* GetDevicePath();
};