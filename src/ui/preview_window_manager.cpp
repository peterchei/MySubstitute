#include "preview_window_manager.h"
#include "capture/frame.h"
#include <iostream>

PreviewWindowManager::PreviewWindowManager()
    : m_hInstance(nullptr)
    , m_hwnd(nullptr)
    , m_initialized(false)
    , m_visible(false)
    , m_memDC(nullptr)
    , m_bitmap(nullptr)
    , m_oldBitmap(nullptr)
    , m_bitmapData(nullptr)
    , m_title(L"MySubstitute - Live Preview")
    , m_refreshRate(30)
    , m_alwaysOnTop(false)
    , m_width(DEFAULT_WIDTH)
    , m_height(DEFAULT_HEIGHT)
{
    ZeroMemory(&m_bitmapInfo, sizeof(m_bitmapInfo));
}

PreviewWindowManager::~PreviewWindowManager() {
    Cleanup();
}

bool PreviewWindowManager::Initialize(HINSTANCE hInstance, FrameCallback frameCallback, FilterChangeCallback filterCallback) {
    if (m_initialized) {
        return true;
    }

    m_hInstance = hInstance;
    m_frameCallback = frameCallback;
    m_filterCallback = filterCallback;

    if (!CreatePreviewWindow(hInstance)) {
        std::wcerr << L"Failed to create preview window" << std::endl;
        return false;
    }

    // Initialize bitmap info for frame rendering
    m_bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    m_bitmapInfo.bmiHeader.biWidth = m_width;
    m_bitmapInfo.bmiHeader.biHeight = -m_height;  // Negative for top-down DIB
    m_bitmapInfo.bmiHeader.biPlanes = 1;
    m_bitmapInfo.bmiHeader.biBitCount = 24;  // 24-bit RGB
    m_bitmapInfo.bmiHeader.biCompression = BI_RGB;
    m_bitmapInfo.bmiHeader.biSizeImage = 0;  // Can be 0 for BI_RGB
    m_bitmapInfo.bmiHeader.biXPelsPerMeter = 0;
    m_bitmapInfo.bmiHeader.biYPelsPerMeter = 0;
    m_bitmapInfo.bmiHeader.biClrUsed = 0;
    m_bitmapInfo.bmiHeader.biClrImportant = 0;

    // Create memory DC for off-screen rendering
    HDC screenDC = GetDC(m_hwnd);
    m_memDC = CreateCompatibleDC(screenDC);
    
    // Create DIB section for frame data
    m_bitmap = CreateDIBSection(m_memDC, &m_bitmapInfo, DIB_RGB_COLORS, 
                               &m_bitmapData, nullptr, 0);
    
    if (!m_bitmap || !m_bitmapData) {
        std::wcerr << L"Failed to create bitmap for frame rendering" << std::endl;
        ReleaseDC(m_hwnd, screenDC);
        return false;
    }
    
    // Select the bitmap into the memory DC
    m_oldBitmap = (HBITMAP)SelectObject(m_memDC, m_bitmap);
    
    if (!CreateControlPanel()) {
        std::wcerr << L"Failed to create control panel" << std::endl;
        return false;
    }

    m_initialized = true;
    return true;
}

bool PreviewWindowManager::ShowPreview() {
    if (!m_initialized || m_visible) {
        return false;
    }

    SetMobilePhoneSize();
    CenterWindow();
    
    ShowWindow(m_hwnd, SW_SHOW);
    UpdateWindow(m_hwnd);
    
    // Start timer for frame updates
    SetTimer(m_hwnd, TIMER_ID, 1000 / m_refreshRate, nullptr);
    
    if (m_alwaysOnTop) {
        SetWindowPos(m_hwnd, HWND_TOPMOST, 0, 0, 0, 0, 
                    SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
    }
    
    m_visible = true;
    return true;
}

bool PreviewWindowManager::HidePreview() {
    if (!m_initialized || !m_visible) {
        return false;
    }

    KillTimer(m_hwnd, TIMER_ID);
    ShowWindow(m_hwnd, SW_HIDE);
    m_visible = false;
    return true;
}

void PreviewWindowManager::SetTitle(const std::wstring& title) {
    m_title = title;
    if (m_hwnd) {
        SetWindowTextW(m_hwnd, m_title.c_str());
    }
}

void PreviewWindowManager::SetRefreshRate(int fps) {
    m_refreshRate = fps;
    if (m_visible && m_hwnd) {
        KillTimer(m_hwnd, TIMER_ID);
        SetTimer(m_hwnd, TIMER_ID, 1000 / m_refreshRate, nullptr);
    }
}

void PreviewWindowManager::SetAlwaysOnTop(bool alwaysOnTop) {
    m_alwaysOnTop = alwaysOnTop;
    if (m_hwnd && m_visible) {
        SetWindowPos(m_hwnd, alwaysOnTop ? HWND_TOPMOST : HWND_NOTOPMOST, 
                    0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    }
}

bool PreviewWindowManager::ProcessMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (hwnd != m_hwnd) {
        return false;
    }

    switch (uMsg) {
        case WM_TIMER:
            if (wParam == TIMER_ID) {
                OnTimer();
                return true;
            }
            break;

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            RenderFrame();
            
            // Copy from memory DC to window DC (only video area, leave space for controls)
            BitBlt(hdc, 0, 0, m_width, m_height, m_memDC, 0, 0, SRCCOPY);
            
            EndPaint(hwnd, &ps);
            return true;
        }

        case WM_COMMAND: {
            int id = LOWORD(wParam);
            int code = HIWORD(wParam);
            OnControlPanelCommand((HWND)lParam, id, code);
            return true;
        }

        case WM_CLOSE:
            HidePreview();
            return true;

        case WM_KEYDOWN:
            if (wParam == VK_ESCAPE) {
                HidePreview();
                return true;
            }
            break;

        case WM_RBUTTONUP: {
            // Show context menu on right-click
            POINT pt = { LOWORD(lParam), HIWORD(lParam) };
            ClientToScreen(hwnd, &pt);
            
            HMENU hMenu = CreatePopupMenu();
            AppendMenuW(hMenu, MF_STRING, 1001, L"Always On Top");
            AppendMenuW(hMenu, MF_STRING, 1002, L"Hide Preview");
            
            if (m_alwaysOnTop) {
                CheckMenuItem(hMenu, 1001, MF_CHECKED);
            }
            
            SetForegroundWindow(hwnd);
            int cmd = TrackPopupMenu(hMenu, TPM_RIGHTBUTTON | TPM_RETURNCMD, 
                                   pt.x, pt.y, 0, hwnd, nullptr);
            
            switch (cmd) {
                case 1001:
                    SetAlwaysOnTop(!m_alwaysOnTop);
                    break;
                case 1002:
                    HidePreview();
                    break;
            }
            
            DestroyMenu(hMenu);
            return true;
        }
    }

    return false;
}

void PreviewWindowManager::Cleanup() {
    if (m_visible) {
        HidePreview();
    }

    if (m_memDC) {
        // Restore the original bitmap before deleting DC
        if (m_oldBitmap) {
            SelectObject(m_memDC, m_oldBitmap);
            m_oldBitmap = nullptr;
        }
        DeleteDC(m_memDC);
        m_memDC = nullptr;
    }

    if (m_bitmap) {
        DeleteObject(m_bitmap);
        m_bitmap = nullptr;
        m_bitmapData = nullptr;
    }

    if (m_hwnd) {
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }

    m_initialized = false;
}

LRESULT CALLBACK PreviewWindowManager::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    PreviewWindowManager* pThis = nullptr;
    
    if (uMsg == WM_NCCREATE) {
        CREATESTRUCT* pCreate = (CREATESTRUCT*)lParam;
        pThis = (PreviewWindowManager*)pCreate->lpCreateParams;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
    } else {
        pThis = (PreviewWindowManager*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    }

    if (pThis && pThis->ProcessMessage(hwnd, uMsg, wParam, lParam)) {
        return 0;
    }

    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

bool PreviewWindowManager::CreatePreviewWindow(HINSTANCE hInstance) {
    const wchar_t* CLASS_NAME = L"MySubstitutePreviewWindow";
    
    WNDCLASSW wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hIcon = LoadIconW(nullptr, (LPCWSTR)IDI_APPLICATION);
    wc.hCursor = LoadCursorW(nullptr, (LPCWSTR)IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszMenuName = nullptr;
    
    if (!RegisterClassW(&wc)) {
        DWORD error = GetLastError();
        if (error != ERROR_CLASS_ALREADY_EXISTS) {
            std::wcerr << L"Failed to register preview window class, error: " << error << std::endl;
            return false;
        }
    }

    // Calculate window size including borders and control panel
    RECT rect = { 0, 0, m_width + CONTROL_PANEL_WIDTH, m_height };
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
    
    int windowWidth = rect.right - rect.left;
    int windowHeight = rect.bottom - rect.top;

    m_hwnd = CreateWindowExW(
        WS_EX_TOOLWINDOW,           // Extended style (tool window)
        CLASS_NAME,                 // Window class name
        m_title.c_str(),           // Window title
        WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX,  // Window style (no maximize)
        CW_USEDEFAULT, CW_USEDEFAULT, windowWidth, windowHeight,
        nullptr,                    // Parent window
        nullptr,                    // Menu
        hInstance,                  // Instance handle
        this                        // Additional application data
    );

    if (!m_hwnd) {
        std::wcerr << L"Failed to create preview window, error: " << GetLastError() << std::endl;
        return false;
    }

    return true;
}

bool PreviewWindowManager::CreateControlPanel() {
    if (!m_hwnd) return false;

    // Create filter selection combo box
    m_filterComboBox = CreateWindowExW(
        0, L"COMBOBOX", nullptr,
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
        m_width + 10, 10, CONTROL_PANEL_WIDTH - 20, 200,
        m_hwnd, (HMENU)1001, m_hInstance, nullptr
    );

    if (!m_filterComboBox) return false;

    // Add filter options
    SendMessageW(m_filterComboBox, CB_ADDSTRING, 0, (LPARAM)L"No Effects");
    SendMessageW(m_filterComboBox, CB_ADDSTRING, 0, (LPARAM)L"Face Filters");
    SendMessageW(m_filterComboBox, CB_ADDSTRING, 0, (LPARAM)L"Virtual Background: Blur");
    SendMessageW(m_filterComboBox, CB_ADDSTRING, 0, (LPARAM)L"Virtual Background: Solid Color");
    SendMessageW(m_filterComboBox, CB_ADDSTRING, 0, (LPARAM)L"Virtual Background: Custom Image");
    SendMessageW(m_filterComboBox, CB_ADDSTRING, 0, (LPARAM)L"Virtual Background: Desktop");
    SendMessageW(m_filterComboBox, CB_ADDSTRING, 0, (LPARAM)L"Virtual Background: Minecraft Pixel");
    SendMessageW(m_filterComboBox, CB_ADDSTRING, 0, (LPARAM)L"Cartoon (Simple)");
    SendMessageW(m_filterComboBox, CB_ADDSTRING, 0, (LPARAM)L"Cartoon (Detailed)");
    SendMessageW(m_filterComboBox, CB_ADDSTRING, 0, (LPARAM)L"Cartoon (Anime)");
    SendMessageW(m_filterComboBox, CB_ADDSTRING, 0, (LPARAM)L"Cartoon Buffered");
    SendMessageW(m_filterComboBox, CB_ADDSTRING, 0, (LPARAM)L"Pixel Art (Minecraft)");
    SendMessageW(m_filterComboBox, CB_ADDSTRING, 0, (LPARAM)L"Pixel Art (Anime)");
    SendMessageW(m_filterComboBox, CB_ADDSTRING, 0, (LPARAM)L"Pixel Art (Retro 16-bit)");
    SendMessageW(m_filterComboBox, CB_ADDSTRING, 0, (LPARAM)L"AI Style: Candy");
    SendMessageW(m_filterComboBox, CB_ADDSTRING, 0, (LPARAM)L"AI Style: Mosaic");
    SendMessageW(m_filterComboBox, CB_ADDSTRING, 0, (LPARAM)L"AI Style: Starry Night");
    SendMessageW(m_filterComboBox, CB_ADDSTRING, 0, (LPARAM)L"AI Style: La Muse");
    SendMessageW(m_filterComboBox, CB_ADDSTRING, 0, (LPARAM)L"AI Style: Feathers");
    SendMessageW(m_filterComboBox, CB_ADDSTRING, 0, (LPARAM)L"Person Detector (Motion Tracker)");
    SendMessageW(m_filterComboBox, CB_SETCURSEL, 0, 0);

    // Create face filter controls
    m_glassesCheckBox = CreateWindowExW(
        0, L"BUTTON", L"Virtual Glasses",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        m_width + 10, 50, CONTROL_PANEL_WIDTH - 20, 20,
        m_hwnd, (HMENU)1002, m_hInstance, nullptr
    );

    m_hatCheckBox = CreateWindowExW(
        0, L"BUTTON", L"Funny Hat",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        m_width + 10, 75, CONTROL_PANEL_WIDTH - 20, 20,
        m_hwnd, (HMENU)1003, m_hInstance, nullptr
    );

    m_speechBubbleCheckBox = CreateWindowExW(
        0, L"BUTTON", L"Speech Bubble",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        m_width + 10, 100, CONTROL_PANEL_WIDTH - 20, 20,
        m_hwnd, (HMENU)1004, m_hInstance, nullptr
    );

    // Speech bubble text input
    CreateWindowExW(
        0, L"STATIC", L"Speech Text:",
        WS_CHILD | WS_VISIBLE,
        m_width + 10, 125, CONTROL_PANEL_WIDTH - 20, 20,
        m_hwnd, nullptr, m_hInstance, nullptr
    );

    m_speechBubbleEdit = CreateWindowExW(
        WS_EX_CLIENTEDGE, L"EDIT", L"Hello Meeting!",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
        m_width + 10, 145, CONTROL_PANEL_WIDTH - 20, 25,
        m_hwnd, (HMENU)1005, m_hInstance, nullptr
    );

    // Initially hide face filter controls
    ShowWindow(m_glassesCheckBox, SW_HIDE);
    ShowWindow(m_hatCheckBox, SW_HIDE);
    ShowWindow(m_speechBubbleCheckBox, SW_HIDE);
    ShowWindow(m_speechBubbleEdit, SW_HIDE);

    return true;
}

void PreviewWindowManager::OnFilterSelectionChanged() {
    if (!m_filterComboBox) return;

    int selection = SendMessageW(m_filterComboBox, CB_GETCURSEL, 0, 0);
    std::cout << "[PreviewWindowManager::OnFilterSelectionChanged] Selection index: " << selection << std::endl;
    
    bool showFaceControls = (selection == 1); // Face Filters

    // Show/hide face filter controls
    ShowWindow(m_glassesCheckBox, showFaceControls ? SW_SHOW : SW_HIDE);
    ShowWindow(m_hatCheckBox, showFaceControls ? SW_SHOW : SW_HIDE);
    ShowWindow(m_speechBubbleCheckBox, showFaceControls ? SW_SHOW : SW_HIDE);
    ShowWindow(m_speechBubbleEdit, showFaceControls ? SW_SHOW : SW_HIDE);

    // Notify callback if set
    if (m_filterCallback) {
        std::string filterName;
        switch (selection) {
            case 0: filterName = "none"; break;
            case 1: filterName = "face_filter"; break;
            case 2: filterName = "virtual_background_blur"; break;
            case 3: filterName = "virtual_background_solid"; break;
            case 4: filterName = "virtual_background_image"; break;
            case 5: filterName = "virtual_background_desktop"; break;
            case 6: filterName = "virtual_background_minecraft"; break;
            case 7: filterName = "cartoon_simple"; break;
            case 8: filterName = "cartoon_detailed"; break;
            case 9: filterName = "cartoon_anime"; break;
            case 10: filterName = "cartoon_buffered"; break;
            case 11: filterName = "pixel_art"; break;
            case 12: filterName = "pixel_art_anime"; break;
            case 13: filterName = "pixel_art_retro"; break;
            case 14: filterName = "style_candy"; break;
            case 15: filterName = "style_mosaic"; break;
            case 16: filterName = "style_starry_night"; break;
            case 17: filterName = "style_la_muse"; break;
            case 18: filterName = "style_feathers"; break;
            case 19: filterName = "person_tracker"; break;
            default: filterName = "none"; break;
        }
        std::cout << "[PreviewWindowManager::OnFilterSelectionChanged] Filter name: '" << filterName << "'" << std::endl;
        m_filterCallback(filterName);
    }
}

void PreviewWindowManager::OnControlPanelCommand(HWND hwnd, int id, int code) {
    if (code != BN_CLICKED && code != CBN_SELCHANGE && code != EN_CHANGE) return;

    switch (id) {
        case 1001: // Filter combo box
            if (code == CBN_SELCHANGE) {
                OnFilterSelectionChanged();
            }
            break;

        case 1002: // Glasses checkbox
            if (code == BN_CLICKED && m_filterCallback) {
                bool checked = (SendMessageW(m_glassesCheckBox, BM_GETCHECK, 0, 0) == BST_CHECKED);
                std::string cmd = checked ? "glasses_on" : "glasses_off";
                m_filterCallback(cmd);
            }
            break;

        case 1003: // Hat checkbox
            if (code == BN_CLICKED && m_filterCallback) {
                bool checked = (SendMessageW(m_hatCheckBox, BM_GETCHECK, 0, 0) == BST_CHECKED);
                std::string cmd = checked ? "hat_on" : "hat_off";
                m_filterCallback(cmd);
            }
            break;

        case 1004: // Speech bubble checkbox
            if (code == BN_CLICKED && m_filterCallback) {
                bool checked = (SendMessageW(m_speechBubbleCheckBox, BM_GETCHECK, 0, 0) == BST_CHECKED);
                std::string cmd = checked ? "speech_on" : "speech_off";
                m_filterCallback(cmd);
            }
            break;

        case 1005: // Speech bubble text edit
            if (code == EN_CHANGE && m_filterCallback) {
                // Only process if face filter is active
                int currentFilter = SendMessageW(m_filterComboBox, CB_GETCURSEL, 0, 0);
                if (currentFilter == 1) {  // Face Filters
                    // Get current speech bubble text
                    wchar_t text[256];
                    GetWindowTextW(m_speechBubbleEdit, text, 256);
                    // Convert wide string to narrow string
                    std::wstring wideText(text);
                    std::string narrowText(wideText.begin(), wideText.end());
                    std::string speechText = "speech_text:" + narrowText;
                    m_filterCallback(speechText);
                }
            }
            break;
    }
}

void PreviewWindowManager::RenderFrame() {
    if (!m_frameCallback || !m_bitmapData) {
        return;
    }

    try {
        Frame frame = m_frameCallback();
        if (!frame.IsValid()) {
            // Fill with black if no frame available - calculate proper stride
            int stride = ((m_width * 3 + 3) & ~3); // 4-byte alignment
            unsigned char* pixels = (unsigned char*)m_bitmapData;
            for (int y = 0; y < m_height; ++y) {
                memset(pixels + y * stride, 0, m_width * 3);
            }
            return;
        }

#if HAVE_OPENCV
        // Convert OpenCV Mat to proper format for Windows bitmap
        cv::Mat displayFrame;
        
        // Handle different input formats
        if (frame.data.channels() == 3) {
            // OpenCV uses BGR, Windows DIB expects BGR - no conversion needed
            frame.data.copyTo(displayFrame);
        } else if (frame.data.channels() == 1) {
            // Convert grayscale to BGR
            cv::cvtColor(frame.data, displayFrame, cv::COLOR_GRAY2BGR);
        } else {
            frame.data.copyTo(displayFrame);
        }
        
        // Resize to fit preview window if needed
        if (displayFrame.cols != m_width || displayFrame.rows != m_height) {
            cv::resize(displayFrame, displayFrame, cv::Size(m_width, m_height), 0, 0, cv::INTER_LINEAR);
        }
        
        // Ensure we have 3-channel RGB data
        if (displayFrame.channels() != 3) {
            std::wcerr << L"Warning: Frame has " << displayFrame.channels() << L" channels, expected 3" << std::endl;
            return;
        }
        
        // Copy pixel data to bitmap with proper stride handling
        // Windows DIB requires 4-byte aligned rows
        int srcStride = displayFrame.step[0]; // OpenCV stride
        int dstStride = ((m_width * 3 + 3) & ~3); // Windows DIB stride (4-byte aligned)
        
        unsigned char* srcData = displayFrame.data;
        unsigned char* dstData = (unsigned char*)m_bitmapData;
        
        for (int y = 0; y < m_height; ++y) {
            // Copy row by row to handle stride differences
            memcpy(dstData + y * dstStride, srcData + y * srcStride, m_width * 3);
        }
#else
        // Fallback: Fill with a pattern if OpenCV is not available
        unsigned char* pixels = (unsigned char*)m_bitmapData;
        for (int y = 0; y < m_height; ++y) {
            for (int x = 0; x < m_width; ++x) {
                int idx = (y * m_width + x) * 3;
                pixels[idx] = 64;      // R
                pixels[idx + 1] = 128; // G  
                pixels[idx + 2] = 192; // B
            }
        }
#endif
    } catch (const std::exception& e) {
        // Fill with error color (red) on exception - handle stride properly
        std::wcerr << L"RenderFrame exception: " << e.what() << std::endl;
        int stride = ((m_width * 3 + 3) & ~3);
        unsigned char* pixels = (unsigned char*)m_bitmapData;
        
        for (int y = 0; y < m_height; ++y) {
            unsigned char* row = pixels + y * stride;
            for (int x = 0; x < m_width; ++x) {
                row[x * 3] = 255;     // R - Red error color
                row[x * 3 + 1] = 0;   // G
                row[x * 3 + 2] = 0;   // B
            }
        }
    }
}

void PreviewWindowManager::OnTimer() {
    InvalidateRect(m_hwnd, nullptr, FALSE);
}

void PreviewWindowManager::SetMobilePhoneSize() {
    RECT rect = { 0, 0, m_width, m_height };
    AdjustWindowRect(&rect, GetWindowLongW(m_hwnd, GWL_STYLE), FALSE);
    
    int windowWidth = rect.right - rect.left;
    int windowHeight = rect.bottom - rect.top;
    
    SetWindowPos(m_hwnd, nullptr, 0, 0, windowWidth, windowHeight, 
                SWP_NOMOVE | SWP_NOZORDER);
}

void PreviewWindowManager::CenterWindow() {
    RECT screenRect;
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &screenRect, 0);
    
    RECT windowRect;
    GetWindowRect(m_hwnd, &windowRect);
    
    int windowWidth = windowRect.right - windowRect.left;
    int windowHeight = windowRect.bottom - windowRect.top;
    
    int x = screenRect.right - windowWidth - 20;  // 20px from right edge
    int y = (screenRect.bottom - screenRect.top - windowHeight) / 2;
    
    SetWindowPos(m_hwnd, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}