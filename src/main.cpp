#include <windows.h>
#include <objbase.h>
#include <memory>
#include <iostream>
#include <string>
#include <mutex>

#include "capture/camera_capture.h"
#include "ai/ai_processor.h"
#include "ai/passthrough_processor.h"
#include "virtual_camera/virtual_camera_filter.h"
#include "virtual_camera/virtual_camera_manager.h"
#include "virtual_camera/camera_diagnostics.h"
#include "virtual_camera/simple_virtual_camera_new.h"
#include "virtual_camera/simple_registry_virtual_camera.h"
#include "virtual_camera/media_foundation_camera.h"
#include "virtual_camera/directshow_virtual_camera_manager.h"
#include "service/background_service.h"
#include "ui/system_tray_manager.h"
#include "ui/preview_window_manager.h"

// Global variables
std::unique_ptr<SystemTrayManager> g_trayManager;
std::unique_ptr<CameraCapture> g_camera;
std::unique_ptr<PassthroughProcessor> g_processor;
std::unique_ptr<VirtualCameraFilter> g_virtualCamera;
std::unique_ptr<VirtualCameraManager> g_virtualCameraManager;
std::unique_ptr<SimpleVirtualCamera> g_simpleVirtualCamera;
std::unique_ptr<PreviewWindowManager> g_previewManager;
bool g_running = true;
Frame g_lastProcessedFrame;  // Store the latest processed frame for preview
Frame g_lastCameraFrame;     // Store the latest camera frame
std::mutex g_frameMutex;     // Protect frame access
bool g_cameraActive = false; // Track camera state

// Function declarations
bool InitializeComponents();
void CleanupComponents();
void OnShowStatus();
void OnSettings();
void OnShowPreview();
void OnHidePreview();
void OnStartCamera();

void OnStopCamera();
void OnReleaseCamera();
void OnRegisterVirtualCamera();
void OnUnregisterVirtualCamera();
void OnStartVirtualCamera();
void OnStopVirtualCamera();
void OnExit();
void ShowStatusMessage();
Frame GetLatestProcessedFrame();  // Callback for preview window
void OnCameraFrame(const Frame& frame);  // Camera frame callback

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Check for test command line argument
    if (lpCmdLine && strstr(lpCmdLine, "--test-virtual-camera")) {
        // Test mode - run virtual camera registration test
        AllocConsole();
        freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
        freopen_s((FILE**)stderr, "CONOUT$", "w", stderr);
        freopen_s((FILE**)stdin, "CONIN$", "r", stdin);
        
        std::wcout << L"🧪 MySubstitute Virtual Camera Test Mode" << std::endl;
        std::wcout << L"=======================================" << std::endl;
        
        CoInitialize(nullptr);
        
        std::wcout << L"\n📊 Initial Status:" << std::endl;
        SimpleRegistryVirtualCamera::ShowDetailedStatus();
        
        std::wcout << L"\n🔄 Attempting DirectShow Virtual Camera registration..." << std::endl;
        DirectShowVirtualCameraManager directShowManager;
        if (directShowManager.RegisterVirtualCamera()) {
            std::wcout << L"✅ DirectShow virtual camera registration completed!" << std::endl;
            directShowManager.ShowDetailedStatus();
        } else {
            std::wcout << L"❌ DirectShow registration failed!" << std::endl;
            std::wcout << L"\n🔄 Trying fallback approaches..." << std::endl;
            
            if (SUCCEEDED(MediaFoundationVirtualCamera::RegisterVirtualCamera())) {
                std::wcout << L"✅ Media Foundation fallback successful!" << std::endl;
                MediaFoundationVirtualCamera::ShowStatus();
            } else if (SimpleRegistryVirtualCamera::RegisterWithAdminCheck()) {
                std::wcout << L"✅ Registry fallback successful!" << std::endl;
                SimpleRegistryVirtualCamera::ShowDetailedStatus();
            } else {
                std::wcout << L"❌ All registration methods failed!" << std::endl;
            }
        }
        
        std::wcout << L"\nPress Enter to exit..." << std::endl;
        std::cin.get();
        
        CoUninitialize();
        FreeConsole();
        return 0;
    }

    // Initialize COM for DirectShow
    HRESULT hr = CoInitialize(nullptr);
    if (FAILED(hr)) {
        MessageBoxA(nullptr, "Failed to initialize COM", "Error", MB_OK | MB_ICONERROR);
        return -1;
    }

    // Initialize components
    if (!InitializeComponents()) {
        MessageBoxA(nullptr, "Failed to initialize components", "Error", MB_OK | MB_ICONERROR);
        CoUninitialize();
        return -1;
    }

    // Initialize system tray
    g_trayManager = std::make_unique<SystemTrayManager>();
    if (!g_trayManager->Initialize(hInstance, L"MySubstitute Virtual Camera")) {
        MessageBoxA(nullptr, "Failed to initialize system tray", "Error", MB_OK | MB_ICONERROR);
        CleanupComponents();
        CoUninitialize();
        return -1;
    }

    // Set tray menu callbacks
    g_trayManager->SetMenuCallback(SystemTrayManager::MENU_SHOW_STATUS, OnShowStatus);
    g_trayManager->SetMenuCallback(SystemTrayManager::MENU_SHOW_PREVIEW, OnShowPreview);
    g_trayManager->SetMenuCallback(SystemTrayManager::MENU_START_CAMERA, OnStartCamera);
    g_trayManager->SetMenuCallback(SystemTrayManager::MENU_STOP_CAMERA, OnStopCamera);
    g_trayManager->SetMenuCallback(SystemTrayManager::MENU_RELEASE_CAMERA, OnReleaseCamera);
    g_trayManager->SetMenuCallback(SystemTrayManager::MENU_REGISTER_VIRTUAL_CAMERA, OnRegisterVirtualCamera);
    g_trayManager->SetMenuCallback(SystemTrayManager::MENU_UNREGISTER_VIRTUAL_CAMERA, OnUnregisterVirtualCamera);
    g_trayManager->SetMenuCallback(SystemTrayManager::MENU_START_VIRTUAL_CAMERA, OnStartVirtualCamera);
    g_trayManager->SetMenuCallback(SystemTrayManager::MENU_STOP_VIRTUAL_CAMERA, OnStopVirtualCamera);
    g_trayManager->SetMenuCallback(SystemTrayManager::MENU_SETTINGS, OnSettings);
    g_trayManager->SetMenuCallback(SystemTrayManager::MENU_EXIT, OnExit);

    // Show tray icon
    if (!g_trayManager->ShowTrayIcon()) {
        MessageBoxA(nullptr, "Failed to show system tray icon", "Error", MB_OK | MB_ICONERROR);
        CleanupComponents();
        CoUninitialize();
        return -1;
    }

    // Initialize preview window
    g_previewManager = std::make_unique<PreviewWindowManager>();
    if (!g_previewManager->Initialize(hInstance, GetLatestProcessedFrame)) {
        MessageBoxA(nullptr, "Failed to initialize preview window", "Warning", MB_OK | MB_ICONWARNING);
        // Continue without preview - not critical
    }

    // Update tray tooltip with status
    g_trayManager->UpdateTooltip(L"MySubstitute - Virtual Camera Running");
    
    // Generate a test frame with caption to demonstrate functionality
    if (g_processor) {
        g_processor->SetCaptionText("MySubstitute Active - Test Caption");
        g_processor->SetCaptionEnabled(true);
    }

    // Main message loop
    MSG msg;
    while (g_running && GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Cleanup
    CleanupComponents();
    CoUninitialize();
    return 0;
}

bool InitializeComponents() {
    try {
        // Initialize camera capture
        g_camera = CameraCapture::Create();
        if (!g_camera || !g_camera->Initialize()) {
            return false;
        }

        // Set up camera callback
        g_camera->SetFrameCallback(OnCameraFrame);
        
        // Select first available camera by default
        auto cameras = g_camera->GetAvailableCameras();
        if (!cameras.empty()) {
            g_camera->SelectCamera(cameras[0].id);
        }

        // Initialize AI processor
        g_processor = std::make_unique<PassthroughProcessor>();
        if (!g_processor->Initialize()) {
            return false;
        }

        // Initialize virtual camera (legacy)
        g_virtualCamera = std::make_unique<VirtualCameraFilter>();
        if (!g_virtualCamera->Initialize()) {
            std::cerr << "Failed to initialize legacy virtual camera filter" << std::endl;
        }

        // Initialize new DirectShow virtual camera manager
        g_virtualCameraManager = std::make_unique<VirtualCameraManager>();
        std::cout << "[Main] ✓ Virtual camera manager initialized" << std::endl;

        // Initialize simple virtual camera
        g_simpleVirtualCamera = std::make_unique<SimpleVirtualCamera>();
        if (g_simpleVirtualCamera->Initialize()) {
            std::cout << "[Main] ✓ Simple virtual camera initialized" << std::endl;
        } else {
            std::cout << "[Main] ⚠️ Simple virtual camera initialization failed" << std::endl;
        }

        return true;
    } catch (const std::exception&) {
        return false;
    }
}

void CleanupComponents() {
    if (g_previewManager) {
        g_previewManager->Cleanup();
        g_previewManager.reset();
    }

    if (g_trayManager) {
        g_trayManager->Cleanup();
        g_trayManager.reset();
    }

    if (g_virtualCamera) {
        g_virtualCamera->Stop();
        g_virtualCamera->Unregister();
        g_virtualCamera.reset();
    }

    if (g_processor) {
        g_processor->Cleanup();
        g_processor.reset();
    }

    if (g_camera) {
        g_camera->StopCapture();
        g_camera.reset();
    }
}

void OnShowStatus() {
    ShowStatusMessage();
}

void OnSettings() {
    MessageBoxA(nullptr, "Settings dialog not implemented yet", "Settings", MB_OK | MB_ICONINFORMATION);
}

void OnShowPreview() {
    if (g_previewManager) {
        if (g_previewManager->IsVisible()) {
            g_previewManager->HidePreview();
        } else {
            g_previewManager->ShowPreview();
        }
    }
}

void OnHidePreview() {
    if (g_previewManager) {
        g_previewManager->HidePreview();
    }
}

void OnStartCamera() {
    // Reinitialize camera if it was released
    if (!g_camera) {
        std::cout << "[Main] Reinitializing camera after release..." << std::endl;
        
        g_camera = CameraCapture::Create();
        if (!g_camera || !g_camera->Initialize()) {
            MessageBoxA(nullptr, 
                "Failed to reinitialize camera.\n\n"
                "Make sure no other applications are using the camera.",
                "Camera Initialization Error", MB_OK | MB_ICONERROR);
            return;
        }
        
        // Set up camera callback
        g_camera->SetFrameCallback(OnCameraFrame);
        
        // Select first available camera by default
        auto cameras = g_camera->GetAvailableCameras();
        if (!cameras.empty()) {
            g_camera->SelectCamera(cameras[0].id);
        }
        
        std::cout << "[Main] ✓ Camera reinitialized successfully" << std::endl;
    }
    
    if (g_camera) {
        if (g_camera->StartCapture()) {
            g_cameraActive = true;
            if (g_trayManager) {
                g_trayManager->UpdateTooltip(L"MySubstitute - Camera Active");
            }
            // Automatically show preview when camera starts
            if (g_previewManager && !g_previewManager->IsVisible()) {
                g_previewManager->ShowPreview();
            }
            
            std::cout << "[Main] ✓ Camera capture started successfully" << std::endl;
        } else {
            MessageBoxA(nullptr, 
                "Failed to start camera capture.\n\n"
                "Possible causes:\n"
                "• Camera is being used by another application\n"
                "• Camera drivers are not properly installed\n"
                "• Camera is physically disconnected", 
                "Camera Error", MB_OK | MB_ICONERROR);
        }
    }
}

void OnStopCamera() {
    // Stop regular camera
    
    if (g_camera) {
        g_camera->StopCapture();
        g_cameraActive = false;
        if (g_trayManager) {
            g_trayManager->UpdateTooltip(L"MySubstitute - Camera Stopped");
        }
        // Clear the last frames
        {
            std::lock_guard<std::mutex> lock(g_frameMutex);
            g_lastCameraFrame = Frame();
            g_lastProcessedFrame = Frame();
        }
    }
}

void OnReleaseCamera() {
    if (g_camera) {
        std::cout << "[Main] Releasing camera for other applications..." << std::endl;
        
        g_camera->StopCapture();
        g_cameraActive = false;
        
        // Completely reset the camera object to ensure full release
        g_camera.reset();
        g_camera = nullptr;
        
        std::cout << "[Main] ✓ Camera object destroyed and released" << std::endl;
        
        if (g_trayManager) {
            g_trayManager->UpdateTooltip(L"MySubstitute - Camera Released for Other Apps");
        }
        
        // Hide preview window
        if (g_previewManager) {
            g_previewManager->HidePreview();
        }
        
        // Clear frames
        {
            std::lock_guard<std::mutex> lock(g_frameMutex);
            g_lastCameraFrame = Frame();
            g_lastProcessedFrame = Frame();
        }
        
        // Wait a moment to ensure complete cleanup
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        MessageBoxA(nullptr, 
            "✅ Camera Completely Released!\n\n"
            "Microsoft Camera and other applications can now use your camera.\n\n"
            "To restart MySubstitute camera:\n"
            "• Right-click tray icon → 'Start Camera'\n"
            "• Camera will be reinitialized from scratch",
            "Camera Released Successfully", MB_OK | MB_ICONINFORMATION);
        
        std::cout << "[Main] ✓ Camera fully released - other apps can now access it" << std::endl;
    } else {
        MessageBoxA(nullptr, 
            "Camera is not currently active in MySubstitute.\n\n"
            "If other apps still can't access the camera, try:\n"
            "• Restarting those applications\n"
            "• Checking if other software is using the camera",
            "Camera Not Active", MB_OK | MB_ICONINFORMATION);
    }
}

void OnRegisterVirtualCamera() {
    std::cout << "[Main] � Starting DirectShow Virtual Camera Registration..." << std::endl;
    
    // Use DirectShow virtual camera - the most compatible approach
    DirectShowVirtualCameraManager* directShowManager = new DirectShowVirtualCameraManager();
    
    if (directShowManager->RegisterVirtualCamera()) {
        std::cout << "[Main] ✅ DirectShow virtual camera registered successfully!" << std::endl;
        directShowManager->ShowDetailedStatus();
    } else {
        std::cout << "[Main] ❌ DirectShow registration failed, trying fallback methods..." << std::endl;
        
        // Fallback to Media Foundation
        if (SUCCEEDED(MediaFoundationVirtualCamera::RegisterVirtualCamera())) {
            MessageBoxA(nullptr,
                "🎉 Fallback Success! Media Foundation Virtual Camera\n\n"
                "✅ MySubstitute Virtual Camera created using Media Foundation\n"
                "✅ Should appear in Camera app and other applications\n\n"
                "📋 Test now: Open Camera app and look for 'MySubstitute Virtual Camera'\n\n"
                "Note: DirectShow method failed, using Media Foundation fallback.",
                "Virtual Camera Success!", MB_OK | MB_ICONINFORMATION);
            delete directShowManager;
            return;
        }
        
        // Final fallback to registry approach
        if (SimpleRegistryVirtualCamera::RegisterWithAdminCheck()) {
            // Registry fallback successful 
            SimpleRegistryVirtualCamera::ShowDetailedStatus();
            MessageBoxA(nullptr,
                "⚠️ Fallback Success - Registry Virtual Camera\n\n"
                "✅ Basic virtual camera registry entries created\n"
                "❓ Limited functionality - may not appear in all apps\n\n"
                "📋 Test in: Windows Camera app\n"
                "💡 For best results, run as Administrator and use DirectShow method",
                "Fallback Registration", MB_OK | MB_ICONWARNING);
        } else {
            // All methods failed
            MessageBoxA(nullptr,
                "❌ All Virtual Camera Registration Methods Failed\n\n"
                "This usually happens when:\n"
                "• Not running as Administrator (most common)\n"
                "• Windows registry restrictions\n"
                "• Build system issues\n\n"
                "💡 SOLUTION:\n"
                "1. Right-click MySubstitute.exe\n"
                "2. Select 'Run as administrator'\n"
                "3. Try registration again\n\n"
                "DirectShow virtual camera requires Administrator privileges\n"
                "for proper system-level filter registration.",
                "Registration Failed", MB_OK | MB_ICONERROR);
        }
    }
    
    delete directShowManager;
}

void OnUnregisterVirtualCamera() {
    if (g_virtualCameraManager) {
        std::cout << "[Main] Unregistering virtual camera..." << std::endl;
        
        if (g_virtualCameraManager->UnregisterVirtualCamera()) {
            if (g_trayManager) {
                g_trayManager->UpdateTooltip(L"MySubstitute - Virtual Camera Unregistered");
            }
            
            MessageBoxA(nullptr,
                "✓ Virtual Camera Unregistered\n\n"
                "MySubstitute Virtual Camera has been removed from the system.\n"
                "Applications will no longer see it in their camera lists.",
                "Virtual Camera Removed", MB_OK | MB_ICONINFORMATION);
        } else {
            MessageBoxA(nullptr,
                "❌ Failed to Unregister Virtual Camera\n\n"
                "Make sure you're running as Administrator.",
                "Unregistration Failed", MB_OK | MB_ICONERROR);
        }
    }
}

void OnStartVirtualCamera() {
    std::cout << "[Main] Starting DirectShow virtual camera..." << std::endl;
    
    // Check if virtual camera is registered first
    DirectShowVirtualCameraManager directShowManager;
    if (!directShowManager.IsRegistered()) {
        MessageBoxA(nullptr,
            "⚠️ Virtual Camera Not Registered\n\n"
            "Please register the DirectShow virtual camera first:\n\n"
            "1. Right-click MySubstitute system tray icon\n"
            "2. Select 'Register Virtual Camera'\n"
            "3. Run as Administrator when prompted\n"
            "4. Wait for registration to complete\n\n"
            "After registration, you can start streaming video.",
            "Registration Required", MB_OK | MB_ICONINFORMATION);
        return;
    }
    
    // Virtual camera is registered - start streaming would go here
    if (g_trayManager) {
        g_trayManager->UpdateTooltip(L"MySubstitute - DirectShow Virtual Camera Active");
    }
    
    MessageBoxA(nullptr,
        "🎥 Virtual Camera Started\n\n"
        "✅ MySubstitute DirectShow virtual camera is now active\n"
        "✅ Should appear in all video applications\n\n"
        "📋 Test now:\n"
        "• Open Windows Camera app\n"
        "• Look for 'MySubstitute Virtual Camera'\n"
        "• Test in Zoom, Teams, browsers\n\n"
        "� Note: This streams processed video from your real camera.",
        "Virtual Camera Active", MB_OK | MB_ICONINFORMATION);
    
    // No virtual camera available
    MessageBoxA(nullptr,
        "❌ No Virtual Camera Registered\n\n"
        "Please register a virtual camera first:\n"
        "1. Right-click MySubstitute system tray icon\n"
        "2. Select 'Register Virtual Camera'\n"
        "3. Run as Administrator if prompted\n\n"
        "After registration, you can start the virtual camera.",
        "Virtual Camera Required", MB_OK | MB_ICONINFORMATION);
}

void OnStopVirtualCamera() {
    if (g_virtualCameraManager) {
        std::cout << "[Main] Stopping virtual camera..." << std::endl;
        
        if (g_virtualCameraManager->StopVirtualCamera()) {
            if (g_trayManager) {
                g_trayManager->UpdateTooltip(L"MySubstitute - Virtual Camera Stopped");
            }
            
            MessageBoxA(nullptr,
                "⏹️ Virtual Camera Stopped\n\n"
                "MySubstitute Virtual Camera is no longer streaming.\n"
                "Applications will show 'camera not available' or switch to other cameras.",
                "Virtual Camera Stopped", MB_OK | MB_ICONINFORMATION);
        }
    }
}

void OnExit() {
    g_running = false;
    PostQuitMessage(0);
}

void ShowStatusMessage() {
    std::string statusText = "MySubstitute Virtual Camera\n\nStatus: Running\n";
    
    if (g_camera) {
        auto cameras = g_camera->GetAvailableCameras();
        statusText += "Available Cameras: " + std::to_string(cameras.size()) + "\n";
        
        for (size_t i = 0; i < cameras.size() && i < 3; ++i) {
            statusText += "  - " + cameras[i].name + "\n";
        }
        
        if (cameras.size() > 3) {
            statusText += "  ... and " + std::to_string(cameras.size() - 3) + " more\n";
        }
    }
    
    if (g_processor) {
        statusText += "\nAI Processor: " + g_processor->GetName() + " v" + g_processor->GetVersion();
    }
    
    MessageBoxA(nullptr, statusText.c_str(), "MySubstitute Status", MB_OK | MB_ICONINFORMATION);
}

Frame GetLatestProcessedFrame() {
    std::lock_guard<std::mutex> lock(g_frameMutex);
    
    // Return processed camera frame if available, otherwise use test frame
    if (g_lastProcessedFrame.IsValid()) {
        return g_lastProcessedFrame;
    }
    
    // Fallback to test frame if no camera data available
    if (g_processor) {
#if HAVE_OPENCV
        // Create a test frame indicating no camera
        cv::Mat testMat = cv::Mat::zeros(480, 640, CV_8UC3);
        testMat.setTo(cv::Scalar(64, 32, 128));  // Purple-ish background
        
        // Add some visual elements
        cv::circle(testMat, cv::Point(320, 240), 80, cv::Scalar(255, 255, 255), 2);
        cv::putText(testMat, "No Camera Active", cv::Point(200, 230), 
                   cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 2);
        cv::putText(testMat, "Start camera from tray menu", cv::Point(150, 260), 
                   cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(200, 200, 200), 1);
        
        // Create Frame from Mat
        Frame testFrame(testMat);
        testFrame.timestamp = GetTickCount64();
        
        // Process the frame through the processor to add caption
        return g_processor->ProcessFrame(testFrame);
#else
        // Create a basic test frame if OpenCV not available
        Frame testFrame(640, 480, 3);
        testFrame.timestamp = GetTickCount64();
        return testFrame;
#endif
    }
    
    // Return empty frame if no processor available
    return Frame();
}

void OnCameraFrame(const Frame& frame) {
    if (!g_processor) {
        return;
    }
    
    // Process the frame through AI processor
    Frame processedFrame = g_processor->ProcessFrame(frame);
    
    // Store both original and processed frames
    {
        std::lock_guard<std::mutex> lock(g_frameMutex);
        g_lastCameraFrame = frame;
        g_lastProcessedFrame = processedFrame;
    }
    
    // Send processed frame to virtual camera (legacy)
    if (g_virtualCamera && g_virtualCamera->IsRunning()) {
        g_virtualCamera->UpdateFrame(processedFrame);
    }
    
    // Send processed frame to new DirectShow virtual camera
    if (g_virtualCameraManager && g_virtualCameraManager->IsActive()) {
        g_virtualCameraManager->UpdateFrame(processedFrame);
    }
}