#pragma once

#include <windows.h>
#include <memory>
#include <functional>
#include <string>

// Forward declarations
struct Frame;

/**
 * Preview window manager for displaying live video feed
 * Creates a mobile phone-sized window showing the processed video
 */
class PreviewWindowManager {
public:
    // Callback function types
    using FrameCallback = std::function<Frame()>;  // Callback to get latest frame
    using FilterChangeCallback = std::function<void(const std::string&)>;  // Callback for filter changes
    
    PreviewWindowManager();
    ~PreviewWindowManager();

    /**
     * Initialize the preview window
     * @param hInstance Application instance handle
     * @param frameCallback Callback to get the latest processed frame
     * @param filterCallback Callback when user changes filter selection
     * @return true if successful
     */
    bool Initialize(HINSTANCE hInstance, FrameCallback frameCallback, FilterChangeCallback filterCallback = nullptr);

    /**
     * Show the preview window
     * @return true if successful
     */
    bool ShowPreview();

    /**
     * Hide the preview window
     * @return true if successful
     */
    bool HidePreview();

    /**
     * Check if preview window is visible
     * @return true if visible
     */
    bool IsVisible() const { return m_visible; }

    /**
     * Update the window title
     * @param title New window title
     */
    void SetTitle(const std::wstring& title);

    /**
     * Set the refresh rate (frames per second)
     * @param fps Frames per second (default: 30)
     */
    void SetRefreshRate(int fps);

    /**
     * Enable/disable always on top behavior
     * @param alwaysOnTop True to keep window on top
     */
    void SetAlwaysOnTop(bool alwaysOnTop);

    /**
     * Get the window handle
     * @return Window handle
     */
    HWND GetWindowHandle() const { return m_hwnd; }

    /**
     * Process Windows messages
     * @param hwnd Window handle
     * @param uMsg Message ID
     * @param wParam Message parameter
     * @param lParam Message parameter
     * @return true if message was handled
     */
    bool ProcessMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    /**
     * Cleanup resources
     */
    void Cleanup();

private:
    // Window procedure for preview window
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    
    // Create the preview window
    bool CreatePreviewWindow(HINSTANCE hInstance);
    
    // Create control panel UI elements
    bool CreateControlPanel();
    
    // Handle control panel events
    void OnFilterSelectionChanged();
    void OnControlPanelCommand(HWND hwnd, int id, int code);
    
    // Render the current frame
    void RenderFrame();
    
    // Timer callback for frame updates
    void OnTimer();
    
    // Window positioning and sizing
    void SetMobilePhoneSize();
    void CenterWindow();
    
    // Constants
    static const int DEFAULT_WIDTH = 640;   // Match virtual camera resolution
    static const int DEFAULT_HEIGHT = 480;
    static const int CONTROL_PANEL_WIDTH = 200;  // Width of control panel
    static const UINT TIMER_ID = 1;
    static const UINT WM_RENDER_FRAME = WM_USER + 100;

    HINSTANCE m_hInstance;
    HWND m_hwnd;
    bool m_initialized;
    bool m_visible;
    FrameCallback m_frameCallback;
    FilterChangeCallback m_filterCallback;
    
    // Control panel UI elements
    HWND m_filterComboBox;
    HWND m_glassesCheckBox;
    HWND m_hatCheckBox;
    HWND m_speechBubbleCheckBox;
    HWND m_speechBubbleEdit;
    
    // Rendering
    HDC m_memDC;
    HBITMAP m_bitmap;
    HBITMAP m_oldBitmap;  // Previously selected bitmap in memory DC
    BITMAPINFO m_bitmapInfo;
    void* m_bitmapData;
    
    // Window properties
    std::wstring m_title;
    int m_refreshRate;  // FPS
    bool m_alwaysOnTop;
    
    // Size and position
    int m_width;
    int m_height;
};