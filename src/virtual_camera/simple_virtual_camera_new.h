#pragma once
#include <windows.h>
#include <string>

/**
 * Simple Virtual Camera using Windows built-in mechanisms
 * This creates a virtual camera that other applications can see and use
 */
class SimpleVirtualCamera {
public:
    SimpleVirtualCamera();
    ~SimpleVirtualCamera();
    
    // Core functionality
    bool Initialize();
    bool Start();
    void Stop();
    void Cleanup();
    
    // Status
    bool IsRegistered() const { return m_isRegistered; }
    bool IsRunning() const { return m_isRunning; }
    
    // Registration with Windows
    bool RegisterWithSystem();
    bool UnregisterFromSystem();
    
    // Check if virtual cameras exist
    static bool CheckForExistingVirtualCameras();
    
private:
    bool m_isRegistered;
    bool m_isRunning;
    std::wstring m_deviceName;
    
    // Windows-specific implementation
    bool CreateRegistryEntries();
    bool RemoveRegistryEntries();
    bool StartCameraService();
    void StopCameraService();
};