#pragma once

#include <windows.h>
#include <string>

/**
 * Virtual Camera Registry Manager
 * Handles proper Windows Registry entries for virtual cameras
 * Uses the same approach as OBS Virtual Camera and other working solutions
 */
class VirtualCameraRegistry
{
public:
    /**
     * Register MySubstitute as a virtual camera device
     * This creates the necessary registry entries for Windows to recognize it
     */
    static bool RegisterVirtualCamera();
    
    /**
     * Unregister MySubstitute virtual camera device
     */
    static bool UnregisterVirtualCamera();
    
    /**
     * Check if the virtual camera is currently registered
     */
    static bool IsVirtualCameraRegistered();
    
    /**
     * List all video capture devices (for debugging)
     */
    static void ListAllCameraDevices();

private:
    /**
     * Create DirectShow filter registry entries
     */
    static bool CreateDirectShowEntries();
    
    /**
     * Create Windows Media Foundation entries
     */
    static bool CreateMediaFoundationEntries();
    
    /**
     * Create device enumeration entries
     */
    static bool CreateDeviceEnumerationEntries();
    
    /**
     * Remove all registry entries
     */
    static bool RemoveAllEntries();
    
    /**
     * Helper function to create registry key with error handling
     */
    static bool CreateRegistryKey(HKEY hRoot, const wchar_t* path, const wchar_t* name, const wchar_t* value);
    
    /**
     * Helper function to delete registry key
     */
    static bool DeleteRegistryKey(HKEY hRoot, const wchar_t* path);
};