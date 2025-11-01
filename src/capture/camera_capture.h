#pragma once

#include <vector>
#include <string>
#include <functional>
#include <memory>

// Forward declarations
struct Frame;
struct CameraDevice;

/**
 * Camera capture interface for accessing physical cameras
 */
class CameraCapture {
public:
    CameraCapture();
    virtual ~CameraCapture();
    
    /**
     * Initialize the camera capture system
     * @return true if successful
     */
    virtual bool Initialize();
    
    /**
     * Start capturing frames from the selected camera
     * @return true if successful
     */
    virtual bool StartCapture();
    
    /**
     * Stop capturing frames
     */
    virtual void StopCapture();
    
    /**
     * Check if currently capturing
     * @return true if capturing
     */
    virtual bool IsCapturing() const;
    
    /**
     * Get list of available cameras
     * @return vector of camera devices
     */
    virtual std::vector<CameraDevice> GetAvailableCameras();
    
    /**
     * Select camera by device ID
     * @param deviceId Camera device ID to select
     * @return true if successful
     */
    virtual bool SelectCamera(int deviceId);
    
    /**
     * Set callback for new frames
     * @param callback Function to call when new frame is available
     */
    virtual void SetFrameCallback(std::function<void(const Frame&)> callback);
    
    /**
     * Set desired frame rate
     * @param fps Frames per second
     * @return true if successful
     */
    virtual bool SetFrameRate(int fps);
    
    /**
     * Set desired resolution
     * @param width Width in pixels
     * @param height Height in pixels
     * @return true if successful
     */
    virtual bool SetResolution(int width, int height);
    
protected:
    bool m_initialized;
    bool m_capturing;
    int m_selectedDevice;
    std::function<void(const Frame&)> m_frameCallback;
};