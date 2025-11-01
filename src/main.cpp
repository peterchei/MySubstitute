#include <windows.h>
#include <objbase.h>
#include <memory>
#include <iostream>
#include <string>

#include "capture/camera_capture.h"
#include "ai/ai_processor.h"
#include "ai/passthrough_processor.h"
#include "virtual_camera/virtual_camera_filter.h"
#include "service/background_service.h"
#include "ui/system_tray_manager.h"

// Global variables
std::unique_ptr<SystemTrayManager> g_trayManager;
std::unique_ptr<CameraCapture> g_camera;
std::unique_ptr<PassthroughProcessor> g_processor;
bool g_running = true;

// Function declarations
bool InitializeComponents();
void CleanupComponents();
void OnShowStatus();
void OnSettings();
void OnExit();
void ShowStatusMessage();

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
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
    g_trayManager->SetMenuCallback(SystemTrayManager::MENU_SETTINGS, OnSettings);
    g_trayManager->SetMenuCallback(SystemTrayManager::MENU_EXIT, OnExit);

    // Show tray icon
    if (!g_trayManager->ShowTrayIcon()) {
        MessageBoxA(nullptr, "Failed to show system tray icon", "Error", MB_OK | MB_ICONERROR);
        CleanupComponents();
        CoUninitialize();
        return -1;
    }

    // Update tray tooltip with status
    g_trayManager->UpdateTooltip(L"MySubstitute - Virtual Camera Running");

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

        // Initialize AI processor
        g_processor = std::make_unique<PassthroughProcessor>();
        if (!g_processor->Initialize()) {
            return false;
        }

        return true;
    } catch (const std::exception&) {
        return false;
    }
}

void CleanupComponents() {
    if (g_trayManager) {
        g_trayManager->Cleanup();
        g_trayManager.reset();
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