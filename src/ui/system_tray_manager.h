#pragma once

#include <windows.h>
#include <shellapi.h>
#include <functional>
#include <string>

/**
 * System tray manager for Windows system tray integration
 * Handles tray icon creation, context menu, and message processing
 */
class SystemTrayManager {
public:
    // Callback function types
    using MenuCallback = std::function<void()>;
    
    // Menu item IDs
    enum MenuItems {
        MENU_SHOW_STATUS = 1001,
        MENU_SHOW_PREVIEW = 1002,
        MENU_START_CAMERA = 1003,
        MENU_STOP_CAMERA = 1004,
        MENU_SETTINGS = 1005,
        MENU_SEPARATOR = 1006,
        MENU_EXIT = 1007
    };

    SystemTrayManager();
    ~SystemTrayManager();

    /**
     * Initialize the system tray icon
     * @param hInstance Application instance handle
     * @param windowTitle Window title for hidden window
     * @return true if successful
     */
    bool Initialize(HINSTANCE hInstance, const std::wstring& windowTitle = L"MySubstitute");

    /**
     * Show the system tray icon
     * @return true if successful
     */
    bool ShowTrayIcon();

    /**
     * Hide the system tray icon
     * @return true if successful
     */
    bool HideTrayIcon();

    /**
     * Update the tray icon tooltip
     * @param tooltip New tooltip text
     * @return true if successful
     */
    bool UpdateTooltip(const std::wstring& tooltip);

    /**
     * Set callback for menu items
     * @param menuId Menu item ID
     * @param callback Function to call when item is selected
     */
    void SetMenuCallback(MenuItems menuId, MenuCallback callback);

    /**
     * Process Windows messages (call from message loop)
     * @param hwnd Window handle
     * @param uMsg Message ID
     * @param wParam Message parameter
     * @param lParam Message parameter
     * @return true if message was handled
     */
    bool ProcessMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    /**
     * Get the hidden window handle
     * @return Window handle
     */
    HWND GetWindowHandle() const { return m_hwnd; }

    /**
     * Cleanup resources
     */
    void Cleanup();

private:
    // Window procedure for hidden window
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    
    // Create and show context menu
    void ShowContextMenu();
    
    // Handle menu command
    void HandleMenuCommand(UINT commandId);
    
    // Create the hidden window
    bool CreateHiddenWindow(HINSTANCE hInstance, const std::wstring& windowTitle);
    
    // Custom message IDs
    static const UINT WM_TRAYICON = WM_USER + 1;
    static const UINT TRAY_ID = 1;

    HINSTANCE m_hInstance;
    HWND m_hwnd;
    NOTIFYICONDATAW m_nid;
    HMENU m_hMenu;
    bool m_initialized;
    bool m_trayIconVisible;
    std::wstring m_windowTitle;
    
    // Callback functions
    MenuCallback m_showStatusCallback;
    MenuCallback m_showPreviewCallback;
    MenuCallback m_startCameraCallback;
    MenuCallback m_stopCameraCallback;
    MenuCallback m_settingsCallback;
    MenuCallback m_exitCallback;
};