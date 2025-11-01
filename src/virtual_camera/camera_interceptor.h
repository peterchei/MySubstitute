#pragma once
#include <windows.h>
#include <dshow.h>
#include <string>
#include <vector>

class CameraInterceptor {
public:
    CameraInterceptor();
    ~CameraInterceptor();
    
    // Find and intercept an existing camera
    bool InterceptCamera(int cameraIndex = -1);  // -1 = auto-select
    void ReleaseCamera();
    
    // Start/stop processing
    bool StartProcessing();
    void StopProcessing();
    
    // Check status
    bool IsIntercepting() const { return m_isIntercepting; }
    std::wstring GetCameraName() const { return m_cameraName; }
    
    // Get list of available cameras
    static std::vector<std::wstring> GetAvailableCameras();
    
private:
    bool m_isIntercepting;
    std::wstring m_cameraName;
    
    // DirectShow components
    IGraphBuilder* m_pGraph;
    ICaptureGraphBuilder2* m_pCapture;
    IMediaControl* m_pControl;
    IBaseFilter* m_pCameraFilter;
    IBaseFilter* m_pProcessingFilter;
    
    bool SetupFilterGraph();
    void CleanupFilterGraph();
    
    // Processing callback
    static HRESULT ProcessFrameCallback(BYTE* pBuffer, long lBufferLen, void* pUserData);
};

// Simple camera list for UI
struct CameraInfo {
    std::wstring name;
    std::wstring devicePath;
    bool isAvailable;
    bool isIntercepted;
};

class SimpleCameraManager {
public:
    static std::vector<CameraInfo> ListAllCameras();
    static bool InterceptCamera(const std::wstring& devicePath);
    static void ReleaseAllCameras();
    static bool TestCameraAccess(const std::wstring& devicePath);
};