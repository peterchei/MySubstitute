#include "camera_capture.h"
#include "frame.h"
#include <iostream>
#include <vector>

#ifdef HAVE_OPENCV
#include <opencv2/opencv.hpp>
#endif

#include <windows.h>
#include <dshow.h>
#include <comutil.h>
#include <thread>
#include <atomic>
#include <mutex>
#include <chrono>

#pragma comment(lib, "strmiids.lib")

// DirectShow camera implementation
class DirectShowCameraCapture : public CameraCapture {
public:
    DirectShowCameraCapture();
    ~DirectShowCameraCapture() override;

    bool Initialize() override;
    bool StartCapture() override;
    void StopCapture() override;
    bool SelectCamera(int deviceId) override;
    void SetFrameCallback(std::function<void(const Frame&)> callback) override;

private:
    void CaptureThread();
    bool CreateGraph();
    void CleanupGraph();

    IGraphBuilder* m_pGraph;
    ICaptureGraphBuilder2* m_pCaptureGraph;
    IBaseFilter* m_pSourceFilter;
    IMediaControl* m_pMediaControl;
    
    std::thread m_captureThread;
    std::atomic<bool> m_shouldCapture;
    std::mutex m_callbackMutex;
    
#ifdef HAVE_OPENCV
    cv::VideoCapture m_openCVCapture;
    bool m_useOpenCV;
#endif
};

// Base class implementation
CameraCapture::CameraCapture() 
    : m_initialized(false), m_capturing(false), m_selectedDevice(-1) {
}

// DirectShow implementation
DirectShowCameraCapture::DirectShowCameraCapture()
    : m_pGraph(nullptr)
    , m_pCaptureGraph(nullptr)
    , m_pSourceFilter(nullptr)
    , m_pMediaControl(nullptr)
    , m_shouldCapture(false)
#ifdef HAVE_OPENCV
    , m_useOpenCV(true)  // Prefer OpenCV for simplicity
#endif
{
}

CameraCapture::~CameraCapture() {
    StopCapture();
}

DirectShowCameraCapture::~DirectShowCameraCapture() {
    StopCapture();
    CleanupGraph();
}

bool CameraCapture::Initialize() {
    if (m_initialized) {
        return true;
    }
    
    // Platform-specific initialization will be handled in derived classes
    m_initialized = true;
    return true;
}

bool CameraCapture::StartCapture() {
    if (!m_initialized) {
        std::cerr << "Camera capture not initialized" << std::endl;
        return false;
    }
    
    if (m_capturing) {
        return true;
    }
    
    if (m_selectedDevice < 0) {
        std::cerr << "No camera selected" << std::endl;
        return false;
    }
    
    // Platform-specific capture start will be implemented in derived classes
    m_capturing = true;
    return true;
}

void CameraCapture::StopCapture() {
    if (!m_capturing) {
        return;
    }
    
    // Platform-specific capture stop will be implemented in derived classes
    m_capturing = false;
}

bool CameraCapture::IsCapturing() const {
    return m_capturing;
}

std::vector<CameraDevice> CameraCapture::GetAvailableCameras() {
    std::vector<CameraDevice> devices;
    
    // Initialize COM if not already initialized
    HRESULT hr = CoInitialize(nullptr);
    bool comInitialized = SUCCEEDED(hr);
    
    // Try to enumerate using DirectShow
    ICreateDevEnum* pDevEnum = nullptr;
    IEnumMoniker* pEnum = nullptr;
    
    hr = CoCreateInstance(CLSID_SystemDeviceEnum, nullptr, CLSCTX_INPROC_SERVER, 
                          IID_ICreateDevEnum, (void**)&pDevEnum);
    
    if (SUCCEEDED(hr)) {
        hr = pDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEnum, 0);
        
        if (SUCCEEDED(hr) && pEnum) {
            IMoniker* pMoniker = nullptr;
            int deviceIndex = 0;
            
            while (pEnum->Next(1, &pMoniker, nullptr) == S_OK) {
                IPropertyBag* pPropBag = nullptr;
                hr = pMoniker->BindToStorage(nullptr, nullptr, IID_IPropertyBag, (void**)&pPropBag);
                
                if (SUCCEEDED(hr)) {
                    VARIANT var;
                    VariantInit(&var);
                    
                    hr = pPropBag->Read(L"Description", &var, nullptr);
                    if (FAILED(hr)) {
                        hr = pPropBag->Read(L"FriendlyName", &var, nullptr);
                    }
                    
                    if (SUCCEEDED(hr)) {
                        // Convert BSTR to std::string
                        std::string name;
                        if (var.bstrVal) {
                            int len = WideCharToMultiByte(CP_UTF8, 0, var.bstrVal, -1, nullptr, 0, nullptr, nullptr);
                            if (len > 0) {
                                std::vector<char> buffer(len);
                                WideCharToMultiByte(CP_UTF8, 0, var.bstrVal, -1, buffer.data(), len, nullptr, nullptr);
                                name = buffer.data();
                            }
                        }
                        
                        if (!name.empty()) {
                            CameraDevice device;
                            device.id = deviceIndex++;
                            device.name = name;
                            device.isAvailable = true;
                            devices.push_back(device);
                        }
                    }
                    
                    VariantClear(&var);
                    pPropBag->Release();
                }
                
                pMoniker->Release();
            }
            
            pEnum->Release();
        }
        
        pDevEnum->Release();
    }
    
    // If no devices found, add dummy devices for testing
    if (devices.empty()) {
        devices.push_back(CameraDevice(0, "Default Camera (simulated)"));
        devices.push_back(CameraDevice(1, "Secondary Camera (simulated)"));
    }
    
    if (comInitialized) {
        CoUninitialize();
    }
    
    return devices;
}

bool CameraCapture::SelectCamera(int deviceId) {
    if (deviceId < 0) {
        return false;
    }
    
    auto cameras = GetAvailableCameras();
    for (const auto& camera : cameras) {
        if (camera.id == deviceId && camera.isAvailable) {
            m_selectedDevice = deviceId;
            return true;
        }
    }
    
    return false;
}

void CameraCapture::SetFrameCallback(std::function<void(const Frame&)> callback) {
    m_frameCallback = callback;
}

bool CameraCapture::SetFrameRate(int fps) {
    if (fps <= 0 || fps > 120) {
        return false;
    }
    
    // Implementation depends on platform-specific capture method
    return true;
}

bool CameraCapture::SetResolution(int width, int height) {
    if (width <= 0 || height <= 0) {
        return false;
    }
    
    // Implementation depends on platform-specific capture method
    return true;
}

std::unique_ptr<CameraCapture> CameraCapture::Create() {
    return std::make_unique<DirectShowCameraCapture>();
}

// DirectShow implementation methods
bool DirectShowCameraCapture::Initialize() {
    if (m_initialized) {
        return true;
    }

#ifdef HAVE_OPENCV
    // Use OpenCV for camera access (simpler than DirectShow for capture)
    m_useOpenCV = true;
    m_initialized = true;
    return true;
#else
    // Fallback to DirectShow if OpenCV not available
    return CreateGraph();
#endif
}

bool DirectShowCameraCapture::StartCapture() {
    if (!m_initialized) {
        std::cerr << "Camera capture not initialized" << std::endl;
        return false;
    }
    
    if (m_capturing) {
        return true;
    }
    
    if (m_selectedDevice < 0) {
        std::cerr << "No camera selected" << std::endl;
        return false;
    }

#ifdef HAVE_OPENCV
    if (m_useOpenCV) {
        // Open camera with OpenCV
        if (!m_openCVCapture.open(m_selectedDevice)) {
            std::cerr << "Failed to open camera " << m_selectedDevice << std::endl;
            return false;
        }
        
        // Set some basic properties
        m_openCVCapture.set(cv::CAP_PROP_FRAME_WIDTH, 640);
        m_openCVCapture.set(cv::CAP_PROP_FRAME_HEIGHT, 480);
        m_openCVCapture.set(cv::CAP_PROP_FPS, 30);
        
        // Start capture thread
        m_shouldCapture = true;
        m_captureThread = std::thread(&DirectShowCameraCapture::CaptureThread, this);
        
        m_capturing = true;
        return true;
    }
#endif
    
    // DirectShow implementation would go here
    std::cerr << "DirectShow capture not implemented yet" << std::endl;
    return false;
}

void DirectShowCameraCapture::StopCapture() {
    if (!m_capturing) {
        return;
    }

    std::cout << "[Camera] Stopping camera capture..." << std::endl;
    m_shouldCapture = false;
    
    if (m_captureThread.joinable()) {
        m_captureThread.join();
        std::cout << "[Camera] Capture thread stopped" << std::endl;
    }

#ifdef HAVE_OPENCV
    if (m_openCVCapture.isOpened()) {
        std::cout << "[Camera] Releasing OpenCV VideoCapture..." << std::endl;
        m_openCVCapture.release();
        
        // Force cleanup and wait for system to release camera
        cv::destroyAllWindows();
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        std::cout << "[Camera] ✓ Camera released completely" << std::endl;
    }
#endif

    m_capturing = false;
}

bool DirectShowCameraCapture::SelectCamera(int deviceId) {
    if (deviceId < 0) {
        return false;
    }
    
    auto cameras = GetAvailableCameras();
    for (const auto& camera : cameras) {
        if (camera.id == deviceId && camera.isAvailable) {
            m_selectedDevice = deviceId;
            return true;
        }
    }
    
    return false;
}

void DirectShowCameraCapture::SetFrameCallback(std::function<void(const Frame&)> callback) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    m_frameCallback = callback;
}

void DirectShowCameraCapture::CaptureThread() {
    std::cout << "Camera capture thread started for device " << m_selectedDevice << std::endl;

#ifdef HAVE_OPENCV
    cv::Mat frame;
    
    while (m_shouldCapture && m_openCVCapture.isOpened()) {
        if (m_openCVCapture.read(frame)) {
            if (!frame.empty()) {
                // Create Frame object
                Frame capturedFrame(frame);
                capturedFrame.timestamp = GetTickCount64();
                
                // Call callback if set
                {
                    std::lock_guard<std::mutex> lock(m_callbackMutex);
                    if (m_frameCallback) {
                        m_frameCallback(capturedFrame);
                    }
                }
            }
        }
        
        // Control frame rate (30 FPS = ~33ms)
        std::this_thread::sleep_for(std::chrono::milliseconds(33));
    }
#endif

    std::cout << "Camera capture thread stopped" << std::endl;
}

bool DirectShowCameraCapture::CreateGraph() {
    // DirectShow graph creation would be implemented here
    // For now, return true as we're using OpenCV
    m_initialized = true;
    return true;
}

void DirectShowCameraCapture::CleanupGraph() {
    // DirectShow cleanup would be implemented here
    if (m_pMediaControl) {
        m_pMediaControl->Release();
        m_pMediaControl = nullptr;
    }
    
    if (m_pSourceFilter) {
        m_pSourceFilter->Release();
        m_pSourceFilter = nullptr;
    }
    
    if (m_pCaptureGraph) {
        m_pCaptureGraph->Release();
        m_pCaptureGraph = nullptr;
    }
    
    if (m_pGraph) {
        m_pGraph->Release();
        m_pGraph = nullptr;
    }
}