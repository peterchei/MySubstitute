#include "system_tray_manager.h"
#include <iostream>

SystemTrayManager::SystemTrayManager()
    : m_hInstance(nullptr)
    , m_hwnd(nullptr)
    , m_hMenu(nullptr)
    , m_initialized(false)
    , m_trayIconVisible(false)
{
    ZeroMemory(&m_nid, sizeof(m_nid));
}

SystemTrayManager::~SystemTrayManager() {
    Cleanup();
}

bool SystemTrayManager::Initialize(HINSTANCE hInstance, const std::wstring& windowTitle) {
    if (m_initialized) {
        return true;
    }

    m_hInstance = hInstance;
    m_windowTitle = windowTitle;

    // Create hidden window for message processing
    if (!CreateHiddenWindow(hInstance, windowTitle)) {
        std::wcerr << L"Failed to create hidden window for system tray" << std::endl;
        return false;
    }

    // Initialize NOTIFYICONDATA structure
    m_nid.cbSize = sizeof(NOTIFYICONDATAW);
    m_nid.hWnd = m_hwnd;
    m_nid.uID = TRAY_ID;
    m_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    m_nid.uCallbackMessage = WM_TRAYICON;
    
    // Use default application icon (can be replaced with custom icon later)
    m_nid.hIcon = LoadIconW(nullptr, (LPCWSTR)IDI_APPLICATION);
    
    // Set initial tooltip
    wcscpy_s(m_nid.szTip, L"MySubstitute Virtual Camera");

    // Create context menu
    m_hMenu = CreatePopupMenu();
    if (!m_hMenu) {
        std::wcerr << L"Failed to create context menu" << std::endl;
        return false;
    }

    // Add menu items
    AppendMenuW(m_hMenu, MF_STRING, MENU_SHOW_STATUS, L"Show Status");
    AppendMenuW(m_hMenu, MF_STRING, MENU_SETTINGS, L"Settings");
    AppendMenuW(m_hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(m_hMenu, MF_STRING, MENU_EXIT, L"Exit");

    m_initialized = true;
    return true;
}

bool SystemTrayManager::ShowTrayIcon() {
    if (!m_initialized || m_trayIconVisible) {
        return false;
    }

    if (Shell_NotifyIconW(NIM_ADD, &m_nid)) {
        m_trayIconVisible = true;
        return true;
    }

    std::wcerr << L"Failed to add system tray icon" << std::endl;
    return false;
}

bool SystemTrayManager::HideTrayIcon() {
    if (!m_initialized || !m_trayIconVisible) {
        return false;
    }

    if (Shell_NotifyIconW(NIM_DELETE, &m_nid)) {
        m_trayIconVisible = false;
        return true;
    }

    std::wcerr << L"Failed to remove system tray icon" << std::endl;
    return false;
}

bool SystemTrayManager::UpdateTooltip(const std::wstring& tooltip) {
    if (!m_initialized || !m_trayIconVisible) {
        return false;
    }

    wcscpy_s(m_nid.szTip, tooltip.c_str());
    return Shell_NotifyIconW(NIM_MODIFY, &m_nid);
}

void SystemTrayManager::SetMenuCallback(MenuItems menuId, MenuCallback callback) {
    switch (menuId) {
        case MENU_SHOW_STATUS:
            m_showStatusCallback = callback;
            break;
        case MENU_SETTINGS:
            m_settingsCallback = callback;
            break;
        case MENU_EXIT:
            m_exitCallback = callback;
            break;
    }
}

bool SystemTrayManager::ProcessMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (hwnd != m_hwnd) {
        return false;
    }

    switch (uMsg) {
        case WM_TRAYICON:
            if (LOWORD(lParam) == WM_RBUTTONUP) {
                ShowContextMenu();
                return true;
            }
            else if (LOWORD(lParam) == WM_LBUTTONDBLCLK) {
                // Double-click to show status
                if (m_showStatusCallback) {
                    m_showStatusCallback();
                }
                return true;
            }
            break;

        case WM_COMMAND:
            HandleMenuCommand(LOWORD(wParam));
            return true;

        case WM_CLOSE:
        case WM_DESTROY:
            // Handle window close (hide instead of destroy)
            HideTrayIcon();
            return true;
    }

    return false;
}

void SystemTrayManager::Cleanup() {
    if (m_trayIconVisible) {
        HideTrayIcon();
    }

    if (m_hMenu) {
        DestroyMenu(m_hMenu);
        m_hMenu = nullptr;
    }

    if (m_hwnd) {
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }

    m_initialized = false;
}

LRESULT CALLBACK SystemTrayManager::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    // Get the SystemTrayManager instance from window user data
    SystemTrayManager* pThis = nullptr;
    
    if (uMsg == WM_NCCREATE) {
        // Store the instance pointer in window user data
        CREATESTRUCT* pCreate = (CREATESTRUCT*)lParam;
        pThis = (SystemTrayManager*)pCreate->lpCreateParams;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
    } else {
        pThis = (SystemTrayManager*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    }

    if (pThis && pThis->ProcessMessage(hwnd, uMsg, wParam, lParam)) {
        return 0;
    }

    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

void SystemTrayManager::ShowContextMenu() {
    if (!m_hMenu || !m_hwnd) {
        return;
    }

    POINT pt;
    GetCursorPos(&pt);

    // Required for proper menu behavior
    SetForegroundWindow(m_hwnd);

    TrackPopupMenu(m_hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, m_hwnd, nullptr);

    // Required cleanup
    PostMessage(m_hwnd, WM_NULL, 0, 0);
}

void SystemTrayManager::HandleMenuCommand(UINT commandId) {
    switch (commandId) {
        case MENU_SHOW_STATUS:
            if (m_showStatusCallback) {
                m_showStatusCallback();
            }
            break;

        case MENU_SETTINGS:
            if (m_settingsCallback) {
                m_settingsCallback();
            }
            break;

        case MENU_EXIT:
            if (m_exitCallback) {
                m_exitCallback();
            }
            break;
    }
}

bool SystemTrayManager::CreateHiddenWindow(HINSTANCE hInstance, const std::wstring& windowTitle) {
    // Register window class
    const wchar_t* CLASS_NAME = L"MySubstituteTrayWindow";
    
    WNDCLASSW wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hIcon = LoadIconW(nullptr, (LPCWSTR)IDI_APPLICATION);
    wc.hCursor = LoadCursorW(nullptr, (LPCWSTR)IDC_ARROW);
    
    if (!RegisterClassW(&wc)) {
        DWORD error = GetLastError();
        if (error != ERROR_CLASS_ALREADY_EXISTS) {
            std::wcerr << L"Failed to register window class, error: " << error << std::endl;
            return false;
        }
    }

    // Create hidden window
    m_hwnd = CreateWindowExW(
        0,                          // Extended window style
        CLASS_NAME,                 // Window class name
        windowTitle.c_str(),        // Window title
        0,                          // Window style (hidden)
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, // Position and size
        nullptr,                    // Parent window
        nullptr,                    // Menu
        hInstance,                  // Instance handle
        this                        // Additional application data
    );

    if (!m_hwnd) {
        std::wcerr << L"Failed to create hidden window, error: " << GetLastError() << std::endl;
        return false;
    }

    return true;
}