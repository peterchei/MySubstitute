#pragma once

#include <memory>
#include <atomic>
#include <string>
#include "../capture/frame.h"

class DirectShowVirtualCameraManager {
private:
    std::atomic<bool> m_isRegistered;
    std::atomic<bool> m_isStreaming;
    std::wstring m_dllPath;
    
public:
    DirectShowVirtualCameraManager();
    ~DirectShowVirtualCameraManager();
    
    // Registration with Windows system
    bool RegisterVirtualCamera();
    bool UnregisterVirtualCamera();
    bool IsRegistered() const { return m_isRegistered; }
    
    // Status information
    std::wstring GetStatus() const;
    void ShowDetailedStatus() const;
    
    // Test if virtual camera appears in device enumeration
    bool TestDeviceVisibility() const;
    
private:
    bool CheckAdminPrivileges() const;
    bool BuildDirectShowDLL();
    bool RegisterDLLWithSystem();
    bool UnregisterDLLFromSystem();
    std::wstring GetDLLPath() const;
};