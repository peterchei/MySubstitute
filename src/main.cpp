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
#include "virtual_camera/obs_virtual_camera_helper.h"
#include "virtual_camera/simple_registry_virtual_camera.h"
#include "virtual_camera/media_foundation_camera.h"
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
        
        std::wcout << L"\n🔄 Attempting Media Foundation registration..." << std::endl;
        if (SUCCEEDED(MediaFoundationVirtualCamera::RegisterVirtualCamera())) {
            std::wcout << L"✅ Media Foundation registration completed!" << std::endl;
            MediaFoundationVirtualCamera::ShowStatus();
        } else {
            std::wcout << L"❌ Media Foundation registration failed!" << std::endl;
            std::wcout << L"\n� Trying fallback registry approach..." << std::endl;
            if (SimpleRegistryVirtualCamera::RegisterWithAdminCheck()) {
                std::wcout << L"✅ Fallback registration completed!" << std::endl;
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
    std::cout << "[Main] 🔍 Analyzing virtual camera options..." << std::endl;
    
    // Check if OBS Virtual Camera is available first
    if (OBSVirtualCameraHelper::IsOBSVirtualCameraInstalled()) {
        // OBS is installed - this is still the best option!
        MessageBoxA(nullptr,
            "🎉 EXCELLENT! OBS Virtual Camera Detected\n\n"
            "✅ OBS Studio with virtual camera is installed\n"
            "✅ This is the BEST solution for virtual cameras\n\n"
            "📋 How to use:\n"
            "1. Open OBS Studio\n"
            "2. Add your camera as a source\n"
            "3. Click 'Start Virtual Camera' in OBS\n"
            "4. 'OBS Virtual Camera' will appear in all apps\n"
            "5. MySubstitute will enhance your real camera feed\n\n"
            "🚀 This setup provides professional-grade virtual camera functionality!",
            "OBS Virtual Camera Found!", MB_OK | MB_ICONINFORMATION);
        return;
    }
    
    // No OBS - try Media Foundation virtual camera first
    std::wcout << L"[Main] OBS not found, attempting Media Foundation virtual camera..." << std::endl;
    
    if (SUCCEEDED(MediaFoundationVirtualCamera::RegisterVirtualCamera())) {
        MessageBoxA(nullptr,
            "🎉 SUCCESS! Media Foundation Virtual Camera Registered\n\n"
            "✅ MySubstitute Virtual Camera created using Windows Media Foundation\n"
            "✅ Should appear in Camera app and other applications\n\n"
            "📋 Test now: Open Camera app and look for 'MySubstitute Virtual Camera'\n"
            "📋 Also test in browsers (Chrome, Firefox, Edge) for video calls\n\n"
            "⚡ This uses modern Windows Media Foundation APIs for better compatibility!",
            "Virtual Camera Success!", MB_OK | MB_ICONINFORMATION);
        return;
    }
    
    // Media Foundation failed - try fallback registry approach
    std::wcout << L"[Main] Media Foundation failed, trying fallback registry method..." << std::endl;
    
    if (SimpleRegistryVirtualCamera::RegisterWithAdminCheck()) {
        // Registration successful - show detailed status
        SimpleRegistryVirtualCamera::ShowDetailedStatus();
        
        // Test visibility
        if (SimpleRegistryVirtualCamera::TestDeviceVisibility()) {
            MessageBoxA(nullptr,
                "🎉 SUCCESS! Virtual Camera Appears in Device List\n\n"
                "✅ MySubstitute Virtual Camera is now visible in:\n"
                "   • Windows Camera app\n"
                "   • Zoom, Teams, Discord, Skype  \n"
                "   • Chrome, Edge, Firefox browsers\n"
                "   • All DirectShow-compatible applications\n\n"
                "⚠️ NOTE: This is a registry-based virtual camera\n"
                "• Device appears in lists but needs actual video source\n"
                "• Use 'Start Virtual Camera' to begin streaming\n"
                "• For full functionality, consider OBS Studio\n\n"
                "📋 Test now: Open Camera app and look for 'MySubstitute Virtual Camera'",
                "Virtual Camera Visible!", MB_OK | MB_ICONINFORMATION);
        } else {
            MessageBoxA(nullptr,
                "⚠️ Registry Entries Created - Testing Needed\n\n"
                "✅ Virtual camera registered in Windows registry\n"
                "❓ Device visibility needs verification\n\n"
                "📋 Test steps:\n"
                "1. Open Windows Camera app\n"
                "2. Look for 'MySubstitute Virtual Camera'\n"
                "3. If not visible, try restarting Camera app\n"
                "4. May need to restart other video applications\n\n"
                "💡 If still not working, OBS Studio is recommended\n"
                "for guaranteed virtual camera functionality",
                "Registration Complete", MB_OK | MB_ICONINFORMATION);
        }
    } else {
        // Registration failed - likely admin rights issue
        int result = MessageBoxA(nullptr,
            "❌ Virtual Camera Registration Failed\n\n"
            "This usually happens when:\n"
            "• Not running as Administrator (most common)\n"
            "• Windows registry restrictions\n"
            "• Antivirus blocking registry changes\n\n"
            "💡 SOLUTIONS:\n\n"
            "1️⃣ RUN AS ADMINISTRATOR (Recommended)\n"
            "• Right-click MySubstitute.exe\n"
            "• Select 'Run as administrator'\n"
            "• Try registration again\n\n"
            "2️⃣ USE OBS STUDIO (Alternative)\n"
            "• Professional virtual camera solution\n"
            "• No admin rights required\n"
            "• Works with all applications\n\n"
            "🔗 Download OBS Studio now?",
            "Registration Failed", MB_YESNO | MB_ICONWARNING);
        
        if (result == IDYES) {
            OBSVirtualCameraHelper::ShowOBSInstallationGuide();
        }
    }
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
    std::cout << "[Main] Starting virtual camera..." << std::endl;
    
    // Check for OBS Virtual Camera first (best option)
    if (OBSVirtualCameraHelper::IsOBSVirtualCameraInstalled()) {
        if (OBSVirtualCameraHelper::StartOBSVirtualCamera()) {
            if (g_trayManager) {
                g_trayManager->UpdateTooltip(L"MySubstitute - OBS Integration Active");
            }
            return;
        }
    }
    
    // Try our simple virtual camera
    if (g_simpleVirtualCamera) {
        if (!g_simpleVirtualCamera->IsRegistered()) {
            MessageBoxA(nullptr,
                "⚠️ No Virtual Camera Available\n\n"
                "No virtual camera infrastructure was found.\n\n"
                "💡 RECOMMENDED SOLUTION:\n"
                "Install OBS Studio (free) which includes a proven\n"
                "virtual camera that works with all applications.\n\n"
                "🔗 Download: https://obsproject.com/\n\n"
                "Alternative: Try 'Register Virtual Camera' first,\n"
                "but OBS is the most reliable option.",
                "Virtual Camera Needed", MB_OK | MB_ICONINFORMATION);
            return;
        }
        
        if (g_simpleVirtualCamera->Start()) {
            if (g_trayManager) {
                g_trayManager->UpdateTooltip(L"MySubstitute - Basic Virtual Camera");
            }
            
            MessageBoxA(nullptr,
                "⚠️ Basic Virtual Camera Started\n\n"
                "✅ MySubstitute virtual camera is running\n"
                "❗ May not work with all applications\n\n"
                "📋 To test:\n"
                "• Open Camera app or Zoom\n"
                "• Look for 'MySubstitute Virtual Camera'\n"
                "• If not visible, install OBS Studio instead\n\n"
                "💡 OBS Studio provides much more reliable virtual camera support.",
                "Basic Virtual Camera", MB_OK | MB_ICONWARNING);
            return;
        }
    }
    
    // No virtual camera available
    OBSVirtualCameraHelper::ShowOBSInstallationGuide();
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