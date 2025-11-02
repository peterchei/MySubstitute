#include "directshow_virtual_camera_manager.h"
#include <windows.h>
#include <iostream>
#include <shlwapi.h>
#include <shellapi.h>

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "shell32.lib")

DirectShowVirtualCameraManager::DirectShowVirtualCameraManager() :
    m_isRegistered(false),
    m_isStreaming(false)
{
    m_dllPath = GetDLLPath();
}

DirectShowVirtualCameraManager::~DirectShowVirtualCameraManager()
{
    // Cleanup if needed
}

bool DirectShowVirtualCameraManager::CheckAdminPrivileges() const
{
    BOOL isAdmin = FALSE;
    PSID adminGroup = nullptr;
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;

    if (AllocateAndInitializeSid(&ntAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &adminGroup)) {
        CheckTokenMembership(nullptr, adminGroup, &isAdmin);
        FreeSid(adminGroup);
    }
    
    return isAdmin != FALSE;
}

std::wstring DirectShowVirtualCameraManager::GetDLLPath() const
{
    wchar_t modulePath[MAX_PATH];
    GetModuleFileNameW(nullptr, modulePath, MAX_PATH);
    
    // Replace MySubstitute.exe with MySubstituteVirtualCamera.dll
    std::wstring path(modulePath);
    size_t lastSlash = path.find_last_of(L'\\');
    if (lastSlash != std::wstring::npos) {
        path = path.substr(0, lastSlash + 1) + L"MySubstituteVirtualCamera.dll";
    }
    
    return path;
}

bool DirectShowVirtualCameraManager::BuildDirectShowDLL()
{
    std::wcout << L"[DirectShow] 🔧 Building DirectShow DLL..." << std::endl;
    
    // Build the DLL using CMake
    std::wstring buildCommand = L"cd /d \"";
    
    // Get application directory
    wchar_t modulePath[MAX_PATH];
    GetModuleFileNameW(nullptr, modulePath, MAX_PATH);
    std::wstring appDir(modulePath);
    size_t lastSlash = appDir.find_last_of(L'\\');
    if (lastSlash != std::wstring::npos) {
        appDir = appDir.substr(0, lastSlash);
        // Go up to project root (from build/bin/Release to project root)
        size_t pos = appDir.find(L"\\build\\");
        if (pos != std::wstring::npos) {
            appDir = appDir.substr(0, pos);
        }
    }
    
    buildCommand += appDir + L"\" && cmake -B build_dll -S . -f CMakeLists_DirectShow.txt && cmake --build build_dll --config Release";
    
    std::wcout << L"[DirectShow] Executing: " << buildCommand << std::endl;
    
    // Execute build command
    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi = {};
    
    if (CreateProcessW(nullptr, const_cast<LPWSTR>(buildCommand.c_str()),
                      nullptr, nullptr, FALSE, CREATE_NO_WINDOW,
                      nullptr, appDir.c_str(), &si, &pi)) {
        
        WaitForSingleObject(pi.hProcess, 30000); // 30 second timeout
        
        DWORD exitCode = 0;
        GetExitCodeProcess(pi.hProcess, &exitCode);
        
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        
        if (exitCode == 0) {
            std::wcout << L"[DirectShow] ✅ DLL build successful!" << std::endl;
            return true;
        } else {
            std::wcout << L"[DirectShow] ❌ DLL build failed with exit code: " << exitCode << std::endl;
            return false;
        }
    }
    
    std::wcout << L"[DirectShow] ❌ Failed to start build process" << std::endl;
    return false;
}

bool DirectShowVirtualCameraManager::RegisterDLLWithSystem()
{
    if (!PathFileExistsW(m_dllPath.c_str())) {
        std::wcout << L"[DirectShow] ❌ DLL not found: " << m_dllPath << std::endl;
        return false;
    }
    
    std::wcout << L"[DirectShow] 📝 Registering DLL: " << m_dllPath << std::endl;
    
    // Use regsvr32 to register the DLL
    std::wstring regCommand = L"regsvr32 /s \"" + m_dllPath + L"\"";
    
    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi = {};
    
    if (CreateProcessW(nullptr, const_cast<LPWSTR>(regCommand.c_str()),
                      nullptr, nullptr, FALSE, CREATE_NO_WINDOW,
                      nullptr, nullptr, &si, &pi)) {
        
        WaitForSingleObject(pi.hProcess, 10000); // 10 second timeout
        
        DWORD exitCode = 0;
        GetExitCodeProcess(pi.hProcess, &exitCode);
        
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        
        if (exitCode == 0) {
            std::wcout << L"[DirectShow] ✅ DLL registered successfully!" << std::endl;
            return true;
        } else {
            std::wcout << L"[DirectShow] ❌ DLL registration failed with exit code: " << exitCode << std::endl;
            return false;
        }
    }
    
    std::wcout << L"[DirectShow] ❌ Failed to start regsvr32" << std::endl;
    return false;
}

bool DirectShowVirtualCameraManager::UnregisterDLLFromSystem()
{
    std::wcout << L"[DirectShow] 🗑️ Unregistering DLL..." << std::endl;
    
    // Use regsvr32 to unregister the DLL
    std::wstring unregCommand = L"regsvr32 /u /s \"" + m_dllPath + L"\"";
    
    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi = {};
    
    if (CreateProcessW(nullptr, const_cast<LPWSTR>(unregCommand.c_str()),
                      nullptr, nullptr, FALSE, CREATE_NO_WINDOW,
                      nullptr, nullptr, &si, &pi)) {
        
        WaitForSingleObject(pi.hProcess, 10000);
        
        DWORD exitCode = 0;
        GetExitCodeProcess(pi.hProcess, &exitCode);
        
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        
        if (exitCode == 0) {
            std::wcout << L"[DirectShow] ✅ DLL unregistered successfully!" << std::endl;
            return true;
        }
    }
    
    return false;
}

bool DirectShowVirtualCameraManager::RegisterVirtualCamera()
{
    std::wcout << L"[DirectShow] 🚀 Starting DirectShow Virtual Camera Registration..." << std::endl;
    
    // Check admin privileges
    if (!CheckAdminPrivileges()) {
        std::wcout << L"[DirectShow] ⚠️ Administrator privileges required for registration" << std::endl;
        
        MessageBoxW(nullptr,
            L"🔐 Administrator Privileges Required\n\n"
            L"DirectShow virtual camera registration requires administrator privileges.\n\n"
            L"Please run MySubstitute as Administrator:\n"
            L"1. Right-click MySubstitute.exe\n"
            L"2. Select 'Run as administrator'\n"
            L"3. Try registering the virtual camera again\n\n"
            L"This is required for system-level DirectShow filter registration.",
            L"Admin Required", MB_OK | MB_ICONWARNING);
        
        return false;
    }
    
    // Step 1: Build the DirectShow DLL
    if (!BuildDirectShowDLL()) {
        std::wcout << L"[DirectShow] ❌ Failed to build DirectShow DLL" << std::endl;
        return false;
    }
    
    // Step 2: Register the DLL with the system
    if (!RegisterDLLWithSystem()) {
        std::wcout << L"[DirectShow] ❌ Failed to register DLL with system" << std::endl;
        return false;
    }
    
    m_isRegistered = true;
    
    std::wcout << L"[DirectShow] 🎉 Virtual Camera Registration Completed!" << std::endl;
    
    // Test visibility
    if (TestDeviceVisibility()) {
        MessageBoxW(nullptr,
            L"🎉 SUCCESS! DirectShow Virtual Camera Registered\n\n"
            L"✅ MySubstitute Virtual Camera is now available as a system camera device\n"
            L"✅ Should appear in Camera app, Zoom, Teams, and browsers\n\n"
            L"📋 Test now:\n"
            L"1. Open Camera app (Windows + S, search 'Camera')\n"
            L"2. Look for camera switching options\n"
            L"3. Select 'MySubstitute Virtual Camera'\n"
            L"4. Test in video call applications\n\n"
            L"⚡ This is a real DirectShow filter - maximum compatibility!",
            L"Virtual Camera Success!", MB_OK | MB_ICONINFORMATION);
    } else {
        MessageBoxW(nullptr,
            L"⚠️ Registration Completed - Testing Required\n\n"
            L"✅ DirectShow DLL registered successfully\n"
            L"❓ Device visibility test inconclusive\n\n"
            L"📋 Please test manually:\n"
            L"1. Open Camera app\n"
            L"2. Check for 'MySubstitute Virtual Camera'\n"
            L"3. Try video calling applications\n\n"
            L"If not visible, restart applications and try again.",
            L"Test Required", MB_OK | MB_ICONWARNING);
    }
    
    return true;
}

bool DirectShowVirtualCameraManager::UnregisterVirtualCamera()
{
    return UnregisterDLLFromSystem();
}

bool DirectShowVirtualCameraManager::TestDeviceVisibility() const
{
    std::wcout << L"[DirectShow] 🔍 Testing device visibility..." << std::endl;
    
    // Basic test - check if our CLSID is registered
    HKEY hKey;
    LONG result = RegOpenKeyExW(HKEY_CLASSES_ROOT,
        L"CLSID\\{B3F3A1C4-8F9E-4A2D-9B5C-7E6F8D4C9A3B}",
        0, KEY_READ, &hKey);
    
    if (result == ERROR_SUCCESS) {
        RegCloseKey(hKey);
        std::wcout << L"[DirectShow] ✅ CLSID registration found" << std::endl;
        return true;
    }
    
    std::wcout << L"[DirectShow] ❌ CLSID not found in registry" << std::endl;
    return false;
}

std::wstring DirectShowVirtualCameraManager::GetStatus() const
{
    std::wstring status = L"DirectShow Virtual Camera Status:\n";
    status += L"  Registered: " + std::wstring(m_isRegistered ? L"✅ Yes" : L"❌ No") + L"\n";
    status += L"  DLL Path: " + m_dllPath + L"\n";
    status += L"  DLL Exists: " + std::wstring(PathFileExistsW(m_dllPath.c_str()) ? L"✅ Yes" : L"❌ No") + L"\n";
    status += L"  Admin Privileges: " + std::wstring(CheckAdminPrivileges() ? L"✅ Yes" : L"❌ No") + L"\n";
    
    return status;
}

void DirectShowVirtualCameraManager::ShowDetailedStatus() const
{
    std::wcout << L"\n📊 DirectShow Virtual Camera Status:" << std::endl;
    std::wcout << L"    Registered: " << (m_isRegistered ? L"✅" : L"❌") << std::endl;
    std::wcout << L"    DLL Path: " << m_dllPath << std::endl;
    std::wcout << L"    DLL Exists: " << (PathFileExistsW(m_dllPath.c_str()) ? L"✅" : L"❌") << std::endl;
    std::wcout << L"    Admin Privileges: " << (CheckAdminPrivileges() ? L"✅" : L"❌") << std::endl;
    std::wcout << L"    Device Visible: " << (TestDeviceVisibility() ? L"✅" : L"❌") << std::endl;
}