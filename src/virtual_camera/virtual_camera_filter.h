#pragma once

#include <string>
#include <functional>
#include <memory>

// Forward declaration
struct Frame;

/**
 * Virtual camera filter interface for creating DirectShow virtual camera
 */
// Forward declaration
class SimpleVirtualCameraFilter;

class VirtualCameraFilter {
private:
    SimpleVirtualCameraFilter* m_pSourceFilter;

public:
    VirtualCameraFilter();
    virtual ~VirtualCameraFilter();
    
    /**
     * Initialize the virtual camera filter
     * @return true if successful
     */
    virtual bool Initialize();
    
    /**
     * Register the virtual camera filter with DirectShow
     * @return true if successful
     */
    virtual bool Register();
    
    /**
     * Unregister the virtual camera filter from DirectShow
     */
    virtual void Unregister();
    
    /**
     * Start the virtual camera
     * @return true if successful
     */
    virtual bool Start();
    
    /**
     * Stop the virtual camera
     */
    virtual void Stop();
    
    /**
     * Check if virtual camera is running
     * @return true if running
     */
    virtual bool IsRunning() const;
    
    /**
     * Update the frame being output by the virtual camera
     * @param frame New frame to output
     */
    virtual void UpdateFrame(const Frame& frame);
    
    /**
     * Set frame source callback
     * @param callback Function to get new frames
     */
    virtual void SetFrameSource(std::function<Frame()> callback);
    
    /**
     * Get virtual camera device name
     * @return Device name that appears in applications
     */
    virtual std::string GetDeviceName() const;
    
    /**
     * Set virtual camera device name
     * @param name Device name to use
     */
    virtual void SetDeviceName(const std::string& name);

protected:
    bool m_initialized;
    bool m_registered;
    bool m_running;
    std::string m_deviceName;
    std::function<Frame()> m_frameSource;
};