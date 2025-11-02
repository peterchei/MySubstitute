#pragma once

#include <windows.h>
#include <memory>
#include <atomic>
#include <string>
#include "../capture/frame.h"

class DirectShowVirtualCameraManager {
private:
    std::atomic<bool> m_isRegistered;
    std::atomic<bool> m_isStreaming;
    std::wstring m_dllPath;
    
    // Shared memory for frame data
    HANDLE m_sharedMemory;
    void* m_sharedBuffer;
    static const size_t SHARED_BUFFER_SIZE = 640 * 480 * 3; // RGB24
    static const wchar_t* SHARED_MEMORY_NAME;
    
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
    
    // Frame streaming interface
    bool IsActive() const { return m_isRegistered && m_isStreaming; }
    void UpdateFrame(const Frame& frame);
    
private:
    bool CheckAdminPrivileges() const;
    bool BuildDirectShowDLL();
    bool RegisterDLLWithSystem();
    bool UnregisterDLLFromSystem();
    std::wstring GetDLLPath() const;
    
    // Shared memory helper methods
    bool CreateSharedMemory();
    void CleanupSharedMemory();
};