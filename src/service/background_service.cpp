#include "background_service.h"
#include <iostream>
#include <thread>
#include <chrono>

BackgroundService::BackgroundService() : m_running(false) {
}

BackgroundService::~BackgroundService() {
    Stop();
}

bool BackgroundService::Start() {
    if (m_running) {
        return true;
    }
    
    UpdateStatus("Starting MySubstitute background service...");
    
    try {
        OnStart();
        m_running = true;
        
        // Start the main service thread
        std::thread serviceThread(&BackgroundService::ServiceMain, this);
        serviceThread.detach();
        
        UpdateStatus("MySubstitute background service started successfully");
        return true;
        
    } catch (const std::exception& e) {
        UpdateStatus("Failed to start service: " + std::string(e.what()));
        return false;
    }
}

void BackgroundService::Stop() {
    if (!m_running) {
        return;
    }
    
    UpdateStatus("Stopping MySubstitute background service...");
    
    m_running = false;
    OnStop();
    
    UpdateStatus("MySubstitute background service stopped");
}

bool BackgroundService::IsRunning() const {
    return m_running;
}

void BackgroundService::SetStatusCallback(std::function<void(const std::string&)> callback) {
    m_statusCallback = callback;
}

std::string BackgroundService::GetStatus() const {
    return m_running ? "Running" : "Stopped";
}

void BackgroundService::ServiceMain() {
    UpdateStatus("Service main loop started");
    
    while (m_running) {
        // Main service loop - this is where the camera processing would happen
        // For now, just sleep to simulate work
        std::this_thread::sleep_for(std::chrono::milliseconds(33)); // ~30 FPS
    }
    
    UpdateStatus("Service main loop ended");
}

void BackgroundService::OnStart() {
    // Initialize camera capture, AI processing, and virtual camera
    // This will be implemented with the actual components
}

void BackgroundService::OnStop() {
    // Cleanup camera capture, AI processing, and virtual camera
    // This will be implemented with the actual components
}

void BackgroundService::UpdateStatus(const std::string& status) {
    if (m_statusCallback) {
        m_statusCallback(status);
    } else {
        std::cout << "[Service] " << status << std::endl;
    }
}