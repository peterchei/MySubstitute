#pragma once

#include <windows.h>
#include <string>
#include <memory>
#include "../capture/frame.h"

/**
 * Simple Virtual Camera using Windows Registry and Shared Memory
 * This creates a working virtual camera that appears in all applications
 */
class SimpleVirtualCamera
{
private:
    bool m_isRegistered;
    bool m_isActive;
    HANDLE m_sharedMemory;
    void* m_sharedBuffer;
    static const size_t SHARED_BUFFER_SIZE = 640 * 480 * 3; // RGB24
    static const wchar_t* SHARED_MEMORY_NAME;
    
public:
    SimpleVirtualCamera();
    ~SimpleVirtualCamera();
    
    /**
     * Register virtual camera in Windows Registry
     * Creates registry entries that make it visible to applications
     */
    bool RegisterCamera();
    
    /**
     * Unregister virtual camera from Windows Registry
     */
    bool UnregisterCamera();
    
    /**
     * Start the virtual camera (create shared memory for frame data)
     */
    bool StartCamera();
    
    /**
     * Stop the virtual camera
     */
    bool StopCamera();
    
    /**
     * Update the virtual camera with a new frame
     */
    bool UpdateFrame(const Frame& frame);
    
    /**
     * Check if camera is registered
     */
    bool IsRegistered() const { return m_isRegistered; }
    
    /**
     * Check if camera is active
     */
    bool IsActive() const { return m_isActive; }
    
    /**
     * Get status string
     */
    std::string GetStatus() const;

private:
    /**
     * Create Windows registry entries for virtual camera
     */
    bool CreateRegistryEntries();
    
    /**
     * Remove Windows registry entries
     */
    bool RemoveRegistryEntries();
    
    /**
     * Create shared memory for frame data
     */
    bool CreateSharedMemory();
    
    /**
     * Cleanup shared memory
     */
    void CleanupSharedMemory();
};