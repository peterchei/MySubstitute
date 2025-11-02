#include "working_directshow_filter.h"
#include <iostream>

// Filter registration template
const AMOVIESETUP_MEDIATYPE sudOpPinTypes =
{
    &MEDIATYPE_Video,       // Major type
    &MEDIASUBTYPE_RGB24     // Minor type
};

const AMOVIESETUP_PIN sudOutputPinDesktop =
{
    L"Output",              // Obsolete, not used.
    FALSE,                  // Is this pin rendered?
    TRUE,                   // Is it an output pin?
    FALSE,                  // Can the filter create zero instances?
    FALSE,                  // Does the filter create multiple instances?
    &CLSID_NULL,            // Obsolete.
    nullptr,                // Obsolete.
    1,                      // Number of media types.
    &sudOpPinTypes          // Pointer to media types.
};

const AMOVIESETUP_FILTER sudMySubstituteVirtualCamera =
{
    &CLSID_MySubstituteVirtualCameraFilter, // Filter CLSID
    L"MySubstitute Virtual Camera",         // String name
    MERIT_DO_NOT_USE,                       // Filter merit
    1,                                      // Number pins
    &sudOutputPinDesktop                    // Pin details
};

// List of class IDs and creator functions for the class factory
CFactoryTemplate g_Templates[] = 
{
    { 
        L"MySubstitute Virtual Camera",
        &CLSID_MySubstituteVirtualCameraFilter,
        CMySubstituteVirtualCameraFilter::CreateInstance,
        nullptr,
        &sudMySubstituteVirtualCamera 
    }
};

int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

// Required DLL exports for DirectShow
STDAPI DllRegisterServer()
{
    return AMovieDllRegisterServer2(TRUE);
}

STDAPI DllUnregisterServer()
{
    return AMovieDllRegisterServer2(FALSE);
}

extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);

BOOL APIENTRY DllMain(HANDLE hModule, DWORD dwReason, LPVOID lpReserved)
{
    return DllEntryPoint((HINSTANCE)(hModule), dwReason, lpReserved);
}

//
// CMySubstituteVirtualCameraFilter Implementation
//
CMySubstituteVirtualCameraFilter::CMySubstituteVirtualCameraFilter(LPUNKNOWN lpunk, HRESULT* phr)
    : CSource(NAME("MySubstitute Virtual Camera Filter"), lpunk, CLSID_MySubstituteVirtualCameraFilter)
{
    std::wcout << L"[VirtualCamera] Creating MySubstitute Virtual Camera Filter" << std::endl;
    
    // Create the output pin
    CMySubstituteOutputPin* pPin = new CMySubstituteOutputPin(phr, this, L"Output");
    if (pPin == nullptr) {
        if (phr) *phr = E_OUTOFMEMORY;
        return;
    }
    
    if (FAILED(*phr)) {
        delete pPin;
        return;
    }
}

CUnknown* WINAPI CMySubstituteVirtualCameraFilter::CreateInstance(LPUNKNOWN lpunk, HRESULT* phr)
{
    std::wcout << L"[VirtualCamera] CreateInstance called" << std::endl;
    
    CUnknown* punk = new CMySubstituteVirtualCameraFilter(lpunk, phr);
    if (punk == nullptr) {
        if (phr) *phr = E_OUTOFMEMORY;
    }
    
    return punk;
}

STDMETHODIMP CMySubstituteVirtualCameraFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
    CheckPointer(ppv, E_POINTER);
    return CSource::NonDelegatingQueryInterface(riid, ppv);
}

//
// CMySubstituteOutputPin Implementation
//
CMySubstituteOutputPin::CMySubstituteOutputPin(HRESULT* phr, CMySubstituteVirtualCameraFilter* pParent, LPCWSTR pPinName)
    : CSourceStream(NAME("MySubstitute Output Pin"), phr, pParent, pPinName)
    , m_iFrameNumber(0)
    , m_rtSampleTime(0)
{
    std::wcout << L"[VirtualCamera] Creating output pin" << std::endl;
    
    // Initialize video info header
    ZeroMemory(&m_VideoInfo, sizeof(VIDEOINFOHEADER));
    
    // Set default format (640x480, RGB24, 30fps)
    m_VideoInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    m_VideoInfo.bmiHeader.biWidth = 640;
    m_VideoInfo.bmiHeader.biHeight = 480;
    m_VideoInfo.bmiHeader.biPlanes = 1;
    m_VideoInfo.bmiHeader.biBitCount = 24;
    m_VideoInfo.bmiHeader.biCompression = BI_RGB;
    m_VideoInfo.bmiHeader.biSizeImage = 640 * 480 * 3;
    
    // 30 FPS = 333333 units (100ns units)
    m_VideoInfo.AvgTimePerFrame = 333333;
}

CMySubstituteOutputPin::~CMySubstituteOutputPin()
{
    std::wcout << L"[VirtualCamera] Destroying output pin" << std::endl;
}

HRESULT CMySubstituteOutputPin::FillBuffer(IMediaSample* pms)
{
    BYTE* pData;
    long lDataLen;
    
    pms->GetPointer(&pData);
    lDataLen = pms->GetSize();
    
    // Generate test frame
    CreateTestFrame(pData, lDataLen);
    
    // Set the sample properties
    REFERENCE_TIME rtStart = m_iFrameNumber * m_VideoInfo.AvgTimePerFrame;
    REFERENCE_TIME rtStop = rtStart + m_VideoInfo.AvgTimePerFrame;
    
    pms->SetTime(&rtStart, &rtStop);
    pms->SetSyncPoint(TRUE);
    
    m_iFrameNumber++;
    
    return NOERROR;
}

HRESULT CMySubstituteOutputPin::DecideBufferSize(IMemAllocator* pAlloc, ALLOCATOR_PROPERTIES* pProperties)
{
    CAutoLock cAutoLock(m_pFilter->pStateLock());
    
    HRESULT hr = NOERROR;
    
    pProperties->cBuffers = 1;
    pProperties->cbBuffer = m_VideoInfo.bmiHeader.biSizeImage;
    
    ALLOCATOR_PROPERTIES Actual;
    hr = pAlloc->SetProperties(pProperties, &Actual);
    
    if (FAILED(hr)) return hr;
    
    if (Actual.cbBuffer < pProperties->cbBuffer) {
        return E_FAIL;
    }
    
    return NOERROR;
}

HRESULT CMySubstituteOutputPin::CheckMediaType(const CMediaType* pMediaType)
{
    if (pMediaType->majortype != MEDIATYPE_Video) {
        return E_INVALIDARG;
    }
    
    if (pMediaType->subtype != MEDIASUBTYPE_RGB24) {
        return E_INVALIDARG;
    }
    
    if (pMediaType->formattype != FORMAT_VideoInfo) {
        return E_INVALIDARG;
    }
    
    return S_OK;
}

HRESULT CMySubstituteOutputPin::GetMediaType(int iPosition, CMediaType* pmt)
{
    if (iPosition < 0) return E_INVALIDARG;
    if (iPosition > 0) return VFW_S_NO_MORE_ITEMS;
    
    return CreateMediaType(pmt, 640, 480, 30);
}

HRESULT CMySubstituteOutputPin::SetMediaType(const CMediaType* pmt)
{
    HRESULT hr = CSourceStream::SetMediaType(pmt);
    if (FAILED(hr)) return hr;
    
    VIDEOINFOHEADER* pvi = (VIDEOINFOHEADER*)pmt->Format();
    if (pvi == nullptr) return E_UNEXPECTED;
    
    m_VideoInfo = *pvi;
    
    std::wcout << L"[VirtualCamera] Media type set: " 
               << m_VideoInfo.bmiHeader.biWidth << L"x" 
               << m_VideoInfo.bmiHeader.biHeight << std::endl;
    
    return S_OK;
}

STDMETHODIMP CMySubstituteOutputPin::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
    if (riid == IID_IAMStreamConfig) {
        return GetInterface((IAMStreamConfig*)this, ppv);
    }
    
    return CSourceStream::NonDelegatingQueryInterface(riid, ppv);
}

// IAMStreamConfig methods
STDMETHODIMP CMySubstituteOutputPin::SetFormat(AM_MEDIA_TYPE* pmt)
{
    CAutoLock lock(&m_cSharedState);
    
    if (pmt == nullptr) return E_POINTER;
    
    CMediaType mt(*pmt);
    return SetMediaType(&mt);
}

STDMETHODIMP CMySubstituteOutputPin::GetFormat(AM_MEDIA_TYPE** ppmt)
{
    *ppmt = CreateMediaType();
    if (*ppmt == nullptr) return E_OUTOFMEMORY;
    
    return CreateMediaType((CMediaType*)*ppmt, 640, 480, 30);
}

STDMETHODIMP CMySubstituteOutputPin::GetNumberOfCapabilities(int* piCount, int* piSize)
{
    *piCount = 1;
    *piSize = sizeof(VIDEO_STREAM_CONFIG_CAPS);
    return S_OK;
}

STDMETHODIMP CMySubstituteOutputPin::GetStreamCaps(int iIndex, AM_MEDIA_TYPE** ppmt, BYTE* pSCC)
{
    if (iIndex != 0) return S_FALSE;
    
    *ppmt = CreateMediaType();
    if (*ppmt == nullptr) return E_OUTOFMEMORY;
    
    HRESULT hr = CreateMediaType((CMediaType*)*ppmt, 640, 480, 30);
    
    if (SUCCEEDED(hr)) {
        VIDEO_STREAM_CONFIG_CAPS* pvscc = (VIDEO_STREAM_CONFIG_CAPS*)pSCC;
        ZeroMemory(pvscc, sizeof(VIDEO_STREAM_CONFIG_CAPS));
        
        pvscc->guid = FORMAT_VideoInfo;
        pvscc->VideoStandard = AnalogVideo_None;
        pvscc->InputSize.cx = 640;
        pvscc->InputSize.cy = 480;
        pvscc->MinCroppingSize.cx = 80;
        pvscc->MinCroppingSize.cy = 60;
        pvscc->MaxCroppingSize.cx = 640;
        pvscc->MaxCroppingSize.cy = 480;
        pvscc->CropGranularityX = 80;
        pvscc->CropGranularityY = 60;
        pvscc->CropAlignX = 0;
        pvscc->CropAlignY = 0;
        pvscc->MinOutputSize.cx = 80;
        pvscc->MinOutputSize.cy = 60;
        pvscc->MaxOutputSize.cx = 640;
        pvscc->MaxOutputSize.cy = 480;
        pvscc->OutputGranularityX = 80;
        pvscc->OutputGranularityY = 60;
        pvscc->StretchTapsX = 0;
        pvscc->StretchTapsY = 0;
        pvscc->ShrinkTapsX = 0;
        pvscc->ShrinkTapsY = 0;
        pvscc->MinFrameInterval = 333333;   // 30 fps
        pvscc->MaxFrameInterval = 1000000;  // 10 fps
        pvscc->MinBitsPerSecond = 1000000;
        pvscc->MaxBitsPerSecond = 50000000;
    }
    
    return hr;
}

// Helper methods
HRESULT CMySubstituteOutputPin::CreateTestFrame(BYTE* pData, long lDataLen)
{
    if (pData == nullptr) return E_POINTER;
    
    int width = m_VideoInfo.bmiHeader.biWidth;
    int height = abs(m_VideoInfo.bmiHeader.biHeight);
    
    // Generate a simple color bar test pattern
    GenerateColorBar(pData, width, height);
    
    return S_OK;
}

void CMySubstituteOutputPin::GenerateColorBar(BYTE* pBuffer, int width, int height)
{
    // Create a simple animated color bar pattern
    int barWidth = width / 8;
    BYTE colors[8][3] = {
        {255, 255, 255},  // White
        {255, 255, 0},    // Yellow  
        {0, 255, 255},    // Cyan
        {0, 255, 0},      // Green
        {255, 0, 255},    // Magenta
        {255, 0, 0},      // Red
        {0, 0, 255},      // Blue
        {0, 0, 0}         // Black
    };
    
    // Add animation by shifting colors based on frame number
    int offset = (m_iFrameNumber / 10) % 8;
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int barIndex = ((x / barWidth) + offset) % 8;
            
            // Calculate pixel position (bottom-up bitmap)
            int pixelIndex = ((height - 1 - y) * width + x) * 3;
            
            pBuffer[pixelIndex + 0] = colors[barIndex][2]; // B
            pBuffer[pixelIndex + 1] = colors[barIndex][1]; // G
            pBuffer[pixelIndex + 2] = colors[barIndex][0]; // R
        }
    }
}

HRESULT CMySubstituteOutputPin::CreateMediaType(CMediaType* pmt, int width, int height, int fps)
{
    pmt->SetType(&MEDIATYPE_Video);
    pmt->SetSubtype(&MEDIASUBTYPE_RGB24);
    pmt->SetFormatType(&FORMAT_VideoInfo);
    pmt->SetTemporalCompression(FALSE);
    
    VIDEOINFOHEADER* pvi = (VIDEOINFOHEADER*)pmt->AllocFormatBuffer(sizeof(VIDEOINFOHEADER));
    if (pvi == nullptr) return E_OUTOFMEMORY;
    
    ZeroMemory(pvi, sizeof(VIDEOINFOHEADER));
    
    pvi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    pvi->bmiHeader.biWidth = width;
    pvi->bmiHeader.biHeight = height;
    pvi->bmiHeader.biPlanes = 1;
    pvi->bmiHeader.biBitCount = 24;
    pvi->bmiHeader.biCompression = BI_RGB;
    pvi->bmiHeader.biSizeImage = width * height * 3;
    
    // Frame rate (100ns units)
    pvi->AvgTimePerFrame = 10000000 / fps;
    
    // Calculate bitrate
    pvi->dwBitRate = (DWORD)(pvi->bmiHeader.biSizeImage * 8 * fps);
    
    return S_OK;
}