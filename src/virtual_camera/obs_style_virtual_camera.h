#pragma once

#include <windows.h>
#include <string>
#include <memory>
#include <thread>
#include <atomic>
#include "../capture/frame.h"

/**
 * OBS-Style Virtual Camera Implementation
 * Creates a virtual camera that mimics OBS Virtual Camera's approach
 */
class OBSStyleVirtualCamera
{
private:
    bool m_isRegistered;
    bool m_isActive;
    std::atomic<bool> m_shouldRun;
    std::thread m_frameThread;
    
    // Shared memory for frame data
    HANDLE m_frameMemory;
    void* m_frameBuffer;
    HANDLE m_frameMutex;
    
    static const size_t FRAME_WIDTH = 640;
    static const size_t FRAME_HEIGHT = 480;
    static const size_t FRAME_SIZE = FRAME_WIDTH * FRAME_HEIGHT * 3; // RGB24
    
public:
    OBSStyleVirtualCamera();
    ~OBSStyleVirtualCamera();
    
    /**
     * Register the virtual camera device
     */
    bool RegisterDevice();
    
    /**
     * Unregister the virtual camera device
     */
    bool UnregisterDevice();
    
    /**
     * Start streaming virtual camera
     */
    bool StartStreaming();
    
    /**
     * Stop streaming virtual camera
     */
    bool StopStreaming();
    
    /**
     * Update frame data
     */
    void UpdateFrame(const Frame& frame);
    
    /**
     * Status functions
     */
    bool IsRegistered() const { return m_isRegistered; }
    bool IsActive() const { return m_isActive; }
    std::string GetStatus() const;
    
private:
    /**
     * Frame streaming thread
     */
    void FrameThreadProc();
    
    /**
     * Create shared resources
     */
    bool CreateSharedResources();
    
    /**
     * Cleanup shared resources
     */
    void CleanupSharedResources();
    
    /**
     * Register in Windows Registry as camera device
     */
    bool RegisterInRegistry();
    
    /**
     * Remove from Windows Registry
     */
    bool UnregisterFromRegistry();
};