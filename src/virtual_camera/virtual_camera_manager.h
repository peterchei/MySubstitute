#pragma once

#include <windows.h>
#include "virtual_camera_directshow.h"
#include "class_factory.h"
#include "virtual_camera_registry.h"
#include "../capture/frame.h"
#include <memory>
#include <thread>
#include <atomic>

/**
 * Virtual Camera Manager
 * High-level interface for managing the DirectShow virtual camera
 */
class VirtualCameraManager
{
private:
    std::unique_ptr<MySubstituteVirtualCameraFilter> m_pFilter;
    std::atomic<bool> m_isRegistered;
    std::atomic<bool> m_isActive;
    
    // Shared memory for inter-process frame communication
    HANDLE m_sharedMemory;
    void* m_sharedBuffer;
    static const size_t SHARED_BUFFER_SIZE = 640 * 480 * 3; // RGB24
    static const wchar_t* SHARED_MEMORY_NAME;
    
public:
    VirtualCameraManager();
    ~VirtualCameraManager();
    
    /**
     * Register the virtual camera with the system
     * Must be run as administrator
     */
    bool RegisterVirtualCamera();
    
    /**
     * Unregister the virtual camera from the system
     * Must be run as administrator
     */
    bool UnregisterVirtualCamera();
    
    /**
     * Start the virtual camera (make it active for streaming)
     */
    bool StartVirtualCamera();
    
    /**
     * Stop the virtual camera
     */
    bool StopVirtualCamera();
    
    /**
     * Update the virtual camera with a new processed frame
     */
    void UpdateFrame(const Frame& frame);
    
    /**
     * Check if virtual camera is registered
     */
    bool IsRegistered() const { return m_isRegistered; }
    
    /**
     * Check if virtual camera is active (streaming)
     */
    bool IsActive() const { return m_isActive; }
    
    /**
     * Get status information for display
     */
    std::string GetStatusString() const;
    
    /**
     * Verify that the virtual camera is properly registered in the system
     */
    bool VerifyRegistration() const;
    
private:
    /**
     * Initialize COM and DirectShow components
     */
    bool InitializeCOM();
    
    // Shared memory helper methods
    bool CreateSharedMemory();
    void CleanupSharedMemory();
    bool WriteFrameToSharedMemory(const Frame& frame);
    
    /**
     * Cleanup COM components
     */
    void CleanupCOM();
};