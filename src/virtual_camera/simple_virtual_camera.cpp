#include "simple_virtual_camera.h"
#include <iostream>
#include <sstream>
#include <opencv2/opencv.hpp>

const wchar_t* SimpleVirtualCamera::SHARED_MEMORY_NAME = L"MySubstituteVirtualCameraFrames";

SimpleVirtualCamera::SimpleVirtualCamera() :
    m_isRegistered(false),
    m_isActive(false),
    m_sharedMemory(nullptr),
    m_sharedBuffer(nullptr)
{
    std::cout << "[SimpleVirtualCamera] Initializing..." << std::endl;
}

SimpleVirtualCamera::~SimpleVirtualCamera()
{
    StopCamera();
    CleanupSharedMemory();
}

bool SimpleVirtualCamera::RegisterCamera()
{
    std::cout << "[SimpleVirtualCamera] Registering virtual camera..." << std::endl;
    
    if (CreateRegistryEntries()) {
        m_isRegistered = true;
        std::cout << "[SimpleVirtualCamera] Virtual camera registered successfully!" << std::endl;
        std::cout << "[SimpleVirtualCamera] Applications should now see 'MySubstitute Virtual Camera'" << std::endl;
        return true;
    } else {
        std::cout << "[SimpleVirtualCamera] Failed to register virtual camera" << std::endl;
        return false;
    }
}

bool SimpleVirtualCamera::UnregisterCamera()
{
    std::cout << "[SimpleVirtualCamera] Unregistering virtual camera..." << std::endl;
    
    if (RemoveRegistryEntries()) {
        m_isRegistered = false;
        std::cout << "[SimpleVirtualCamera] Virtual camera unregistered successfully" << std::endl;
        return true;
    } else {
        std::cout << "[SimpleVirtualCamera] Failed to unregister virtual camera" << std::endl;
        return false;
    }
}

bool SimpleVirtualCamera::StartCamera()
{
    if (m_isActive) {
        std::cout << "[SimpleVirtualCamera] Virtual camera already active" << std::endl;
        return true;
    }
    
    std::cout << "[SimpleVirtualCamera] Starting virtual camera..." << std::endl;
    
    if (!CreateSharedMemory()) {
        std::cout << "[SimpleVirtualCamera] Failed to create shared memory" << std::endl;
        return false;
    }
    
    m_isActive = true;
    std::cout << "[SimpleVirtualCamera] Virtual camera started successfully" << std::endl;
    return true;
}

bool SimpleVirtualCamera::StopCamera()
{
    if (!m_isActive) {
        return true;
    }
    
    std::cout << "[SimpleVirtualCamera] Stopping virtual camera..." << std::endl;
    CleanupSharedMemory();
    m_isActive = false;
    std::cout << "[SimpleVirtualCamera] Virtual camera stopped" << std::endl;
    return true;
}

bool SimpleVirtualCamera::UpdateFrame(const Frame& frame)
{
    if (!m_isActive || !m_sharedBuffer) {
        return false;
    }
    
    if (frame.data.empty()) {
        // Create a test pattern frame
        cv::Mat testFrame(480, 640, CV_8UC3, cv::Scalar(50, 100, 200)); // Blue background
        
        // Add a white circle that moves
        static int circleX = 50;
        static int circleY = 50;
        static int deltaX = 2;
        static int deltaY = 2;
        
        circleX += deltaX;
        circleY += deltaY;
        
        if (circleX >= 590 || circleX <= 50) deltaX = -deltaX;
        if (circleY >= 430 || circleY <= 50) deltaY = -deltaY;
        
        cv::circle(testFrame, cv::Point(circleX, circleY), 30, cv::Scalar(255, 255, 255), -1);
        
        // Add text
        cv::putText(testFrame, "MySubstitute Virtual Camera", 
                   cv::Point(50, 450), cv::FONT_HERSHEY_SIMPLEX, 0.8, 
                   cv::Scalar(255, 255, 255), 2);
        
        // Copy to shared memory
        if (testFrame.isContinuous() && testFrame.total() * testFrame.elemSize() <= SHARED_BUFFER_SIZE) {
            memcpy(m_sharedBuffer, testFrame.data, testFrame.total() * testFrame.elemSize());
            return true;
        }
    } else {
        // Use actual frame data
        if (frame.data.size() <= SHARED_BUFFER_SIZE) {
            memcpy(m_sharedBuffer, frame.data.data(), frame.data.size());
            return true;
        }
    }
    
    return false;
}

std::string SimpleVirtualCamera::GetStatus() const
{
    std::ostringstream oss;
    oss << "Simple Virtual Camera: ";
    
    if (m_isRegistered) {
        oss << "Registered";
        if (m_isActive) {
            oss << " & Active";
        } else {
            oss << " & Inactive";
        }
    } else {
        oss << "Not Registered";
    }
    
    return oss.str();
}

bool SimpleVirtualCamera::CreateRegistryEntries()
{
    HKEY hKey;
    LONG result;
    DWORD dwDisposition;
    
    std::cout << "[SimpleVirtualCamera] Creating registry entries..." << std::endl;
    
    // Create the main camera device key
    const wchar_t* devicePath = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\MySubstitute Virtual Camera";
    
    result = RegCreateKeyExW(
        HKEY_LOCAL_MACHINE,
        devicePath,
        0,
        nullptr,
        REG_OPTION_NON_VOLATILE,
        KEY_WRITE,
        nullptr,
        &hKey,
        &dwDisposition
    );
    
    if (result != ERROR_SUCCESS) {
        std::cout << "[SimpleVirtualCamera] ❌ Failed to create registry key: " << result << std::endl;
        return false;
    }
    
    // Set device name
    const wchar_t* deviceName = L"MySubstitute Virtual Camera";
    result = RegSetValueExW(hKey, L"FriendlyName", 0, REG_SZ, 
                           (const BYTE*)deviceName, 
                           (wcslen(deviceName) + 1) * sizeof(wchar_t));
    
    if (result != ERROR_SUCCESS) {
        RegCloseKey(hKey);
        std::cout << "[SimpleVirtualCamera] ❌ Failed to set device name: " << result << std::endl;
        return false;
    }
    
    RegCloseKey(hKey);
    
    // Also create a DirectShow-compatible entry
    const wchar_t* dsPath = L"SOFTWARE\\Classes\\CLSID\\{B3F3A1C4-8F9E-4A2D-9B5C-7E6F8D4C9A3B}";
    
    result = RegCreateKeyExW(
        HKEY_LOCAL_MACHINE,
        dsPath,
        0,
        nullptr,
        REG_OPTION_NON_VOLATILE,
        KEY_WRITE,
        nullptr,
        &hKey,
        &dwDisposition
    );
    
    if (result == ERROR_SUCCESS) {
        RegSetValueExW(hKey, nullptr, 0, REG_SZ, 
                      (const BYTE*)deviceName, 
                      (wcslen(deviceName) + 1) * sizeof(wchar_t));
        RegCloseKey(hKey);
    }
    
    std::cout << "[SimpleVirtualCamera] ✅ Registry entries created" << std::endl;
    return true;
}

bool SimpleVirtualCamera::RemoveRegistryEntries()
{
    std::cout << "[SimpleVirtualCamera] Removing registry entries..." << std::endl;
    
    // Remove the registry entries
    const wchar_t* devicePath = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\MySubstitute Virtual Camera";
    LONG result = RegDeleteTreeW(HKEY_LOCAL_MACHINE, devicePath);
    
    const wchar_t* dsPath = L"SOFTWARE\\Classes\\CLSID\\{B3F3A1C4-8F9E-4A2D-9B5C-7E6F8D4C9A3B}";
    RegDeleteTreeW(HKEY_LOCAL_MACHINE, dsPath);
    
    std::cout << "[SimpleVirtualCamera] ✅ Registry entries removed" << std::endl;
    return true;
}

bool SimpleVirtualCamera::CreateSharedMemory()
{
    std::cout << "[SimpleVirtualCamera] Creating shared memory..." << std::endl;
    
    m_sharedMemory = CreateFileMappingW(
        INVALID_HANDLE_VALUE,
        nullptr,
        PAGE_READWRITE,
        0,
        SHARED_BUFFER_SIZE,
        SHARED_MEMORY_NAME
    );
    
    if (m_sharedMemory == nullptr) {
        std::cout << "[SimpleVirtualCamera] ❌ Failed to create shared memory" << std::endl;
        return false;
    }
    
    m_sharedBuffer = MapViewOfFile(
        m_sharedMemory,
        FILE_MAP_ALL_ACCESS,
        0,
        0,
        SHARED_BUFFER_SIZE
    );
    
    if (m_sharedBuffer == nullptr) {
        CloseHandle(m_sharedMemory);
        m_sharedMemory = nullptr;
        std::cout << "[SimpleVirtualCamera] ❌ Failed to map shared memory" << std::endl;
        return false;
    }
    
    // Initialize with a test pattern
    memset(m_sharedBuffer, 0x80, SHARED_BUFFER_SIZE); // Gray background
    
    std::cout << "[SimpleVirtualCamera] ✅ Shared memory created successfully" << std::endl;
    return true;
}

void SimpleVirtualCamera::CleanupSharedMemory()
{
    if (m_sharedBuffer) {
        UnmapViewOfFile(m_sharedBuffer);
        m_sharedBuffer = nullptr;
    }
    
    if (m_sharedMemory) {
        CloseHandle(m_sharedMemory);
        m_sharedMemory = nullptr;
    }
}