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

bool PreviewWindowManager::Initialize(HINSTANCE hInstance, FrameCallback frameCallback) {
    if (m_initialized) {
        return true;
    }

    m_hInstance = hInstance;
    m_frameCallback = frameCallback;

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
    
    SelectObject(m_memDC, m_bitmap);
    ReleaseDC(m_hwnd, screenDC);

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
            
            // Copy from memory DC to window DC
            BitBlt(hdc, 0, 0, m_width, m_height, m_memDC, 0, 0, SRCCOPY);
            
            EndPaint(hwnd, &ps);
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

    if (m_bitmap) {
        DeleteObject(m_bitmap);
        m_bitmap = nullptr;
        m_bitmapData = nullptr;
    }

    if (m_memDC) {
        DeleteDC(m_memDC);
        m_memDC = nullptr;
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

    // Calculate window size including borders
    RECT rect = { 0, 0, m_width, m_height };
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
            // Convert BGR to RGB (OpenCV uses BGR, Windows DIB uses RGB)
            cv::cvtColor(frame.data, displayFrame, cv::COLOR_BGR2RGB);
        } else if (frame.data.channels() == 1) {
            // Convert grayscale to RGB
            cv::cvtColor(frame.data, displayFrame, cv::COLOR_GRAY2RGB);
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
        
        int stride = ((m_width * 3 + 3) & ~3); // 4-byte alignment
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