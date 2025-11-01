#pragma once

#include <windows.h>
#include <string>

/**
 * DirectShow Virtual Camera Loader
 * Creates a working virtual camera by leveraging existing Windows infrastructure
 */
class VirtualCameraLoader
{
public:
    /**
     * Install MySubstitute as a virtual camera using Windows built-in mechanisms
     */
    bool InstallVirtualCamera();
    
    /**
     * Uninstall the virtual camera
     */
    bool UninstallVirtualCamera();
    
    /**
     * Check if OBS Virtual Camera is installed (we can leverage it)
     */
    bool IsOBSVirtualCameraAvailable();
    
    /**
     * Test if virtual camera is visible
     */
    bool TestVirtualCameraVisibility();

private:
    /**
     * Copy OBS Virtual Camera approach if available
     */
    bool CopyOBSRegistration();
    
    /**
     * Use Windows 10+ virtual camera APIs
     */
    bool UseWindows10VirtualCamera();
    
    /**
     * Create a pass-through filter that uses existing cameras
     */
    bool CreatePassThroughFilter();
    
    /**
     * Register using Windows Media Foundation instead of DirectShow
     */
    bool RegisterWithMediaFoundation();
};