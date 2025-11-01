#include "virtual_camera_directshow.h"
#include <strsafe.h>
#include <dvdmedia.h>
#include <mmreg.h>
#include <olectl.h>
#include <iostream>

// DirectShow time units (100ns intervals)
#define UNITS 10000000

// Static frame rate (30 FPS)
const REFERENCE_TIME FPS_30 = UNITS / 30;

//=============================================================================
// MySubstituteVirtualCameraFilter Implementation
//=============================================================================

MySubstituteVirtualCameraFilter::MySubstituteVirtualCameraFilter() :
    m_cRef(1),
    m_State(State_Stopped),
    m_pClock(nullptr),
    m_pGraph(nullptr),
    m_pOutputPin(nullptr)
{
    InitializeCriticalSection(&m_FilterLock);
    wcscpy_s(m_wszName, L"MySubstitute Virtual Camera");
    
    // Create output pin
    m_pOutputPin = new MySubstituteOutputPin(this);
}

MySubstituteVirtualCameraFilter::~MySubstituteVirtualCameraFilter()
{
    if (m_pOutputPin) {
        delete m_pOutputPin;
    }
    
    if (m_pClock) {
        m_pClock->Release();
    }
    
    if (m_pGraph) {
        m_pGraph->Release();
    }
    
    DeleteCriticalSection(&m_FilterLock);
}

MySubstituteVirtualCameraFilter* MySubstituteVirtualCameraFilter::CreateInstance()
{
    return new MySubstituteVirtualCameraFilter();
}

// IUnknown methods
STDMETHODIMP MySubstituteVirtualCameraFilter::QueryInterface(REFIID riid, void **ppv)
{
    if (!ppv) return E_POINTER;
    
    if (riid == IID_IUnknown) {
        *ppv = static_cast<IUnknown*>(static_cast<IBaseFilter*>(this));
    } else if (riid == IID_IPersist) {
        *ppv = static_cast<IPersist*>(this);
    } else if (riid == IID_IMediaFilter) {
        *ppv = static_cast<IMediaFilter*>(this);
    } else if (riid == IID_IBaseFilter) {
        *ppv = static_cast<IBaseFilter*>(this);
    } else if (riid == IID_IAMStreamConfig) {
        *ppv = static_cast<IAMStreamConfig*>(this);
    } else if (riid == IID_IKsPropertySet) {
        *ppv = static_cast<IKsPropertySet*>(this);
    } else {
        *ppv = nullptr;
        return E_NOINTERFACE;
    }
    
    AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG) MySubstituteVirtualCameraFilter::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) MySubstituteVirtualCameraFilter::Release()
{
    LONG cRef = InterlockedDecrement(&m_cRef);
    if (cRef == 0) {
        delete this;
    }
    return cRef;
}

// IPersist method
STDMETHODIMP MySubstituteVirtualCameraFilter::GetClassID(CLSID *pClsID)
{
    if (!pClsID) return E_POINTER;
    *pClsID = CLSID_MySubstituteVirtualCamera;
    return S_OK;
}

// IMediaFilter methods
STDMETHODIMP MySubstituteVirtualCameraFilter::Stop()
{
    Lock();
    
    if (m_State != State_Stopped) {
        if (m_pOutputPin) {
            m_pOutputPin->Stop();
        }
        m_State = State_Stopped;
    }
    
    Unlock();
    return S_OK;
}

STDMETHODIMP MySubstituteVirtualCameraFilter::Pause()
{
    Lock();
    
    if (m_State == State_Stopped) {
        if (m_pOutputPin) {
            m_pOutputPin->Pause();
        }
    }
    
    m_State = State_Paused;
    Unlock();
    return S_OK;
}

STDMETHODIMP MySubstituteVirtualCameraFilter::Run(REFERENCE_TIME tStart)
{
    Lock();
    
    if (m_State == State_Stopped) {
        Pause();
    }
    
    if (m_pOutputPin) {
        m_pOutputPin->Run(tStart);
    }
    
    m_State = State_Running;
    Unlock();
    return S_OK;
}

STDMETHODIMP MySubstituteVirtualCameraFilter::GetState(DWORD dwMilliSecsTimeout, FILTER_STATE *State)
{
    if (!State) return E_POINTER;
    
    Lock();
    *State = m_State;
    Unlock();
    
    return S_OK;
}

STDMETHODIMP MySubstituteVirtualCameraFilter::SetSyncSource(IReferenceClock *pClock)
{
    Lock();
    
    if (m_pClock) {
        m_pClock->Release();
    }
    
    m_pClock = pClock;
    if (m_pClock) {
        m_pClock->AddRef();
    }
    
    Unlock();
    return S_OK;
}

STDMETHODIMP MySubstituteVirtualCameraFilter::GetSyncSource(IReferenceClock **pClock)
{
    if (!pClock) return E_POINTER;
    
    Lock();
    *pClock = m_pClock;
    if (m_pClock) {
        m_pClock->AddRef();
    }
    Unlock();
    
    return S_OK;
}

// IBaseFilter methods
STDMETHODIMP MySubstituteVirtualCameraFilter::EnumPins(IEnumPins **ppEnum)
{
    if (!ppEnum) return E_POINTER;
    
    *ppEnum = new MySubstitutePinEnum(this);
    return S_OK;
}

STDMETHODIMP MySubstituteVirtualCameraFilter::FindPin(LPCWSTR Id, IPin **ppPin)
{
    if (!ppPin) return E_POINTER;
    
    if (wcscmp(Id, L"Output") == 0) {
        *ppPin = m_pOutputPin;
        m_pOutputPin->AddRef();
        return S_OK;
    }
    
    *ppPin = nullptr;
    return VFW_E_NOT_FOUND;
}

STDMETHODIMP MySubstituteVirtualCameraFilter::QueryFilterInfo(FILTER_INFO *pInfo)
{
    if (!pInfo) return E_POINTER;
    
    wcscpy_s(pInfo->achName, m_wszName);
    pInfo->pGraph = m_pGraph;
    if (m_pGraph) {
        m_pGraph->AddRef();
    }
    
    return S_OK;
}

STDMETHODIMP MySubstituteVirtualCameraFilter::JoinFilterGraph(IFilterGraph *pGraph, LPCWSTR pName)
{
    Lock();
    
    if (m_pGraph) {
        m_pGraph->Release();
    }
    
    m_pGraph = pGraph;
    if (m_pGraph) {
        m_pGraph->AddRef();
    }
    
    if (pName) {
        wcscpy_s(m_wszName, pName);
    }
    
    Unlock();
    return S_OK;
}

STDMETHODIMP MySubstituteVirtualCameraFilter::QueryVendorInfo(LPWSTR *pVendorInfo)
{
    if (!pVendorInfo) return E_POINTER;
    
    *pVendorInfo = (LPWSTR)CoTaskMemAlloc(sizeof(WCHAR) * 32);
    if (!*pVendorInfo) return E_OUTOFMEMORY;
    
    wcscpy_s(*pVendorInfo, 32, L"MySubstitute");
    return S_OK;
}

// IAMStreamConfig methods
STDMETHODIMP MySubstituteVirtualCameraFilter::SetFormat(AM_MEDIA_TYPE *pmt)
{
    if (m_pOutputPin) {
        return m_pOutputPin->SetFormat(pmt);
    }
    return E_UNEXPECTED;
}

STDMETHODIMP MySubstituteVirtualCameraFilter::GetFormat(AM_MEDIA_TYPE **ppmt)
{
    if (m_pOutputPin) {
        return m_pOutputPin->GetFormat(ppmt);
    }
    return E_UNEXPECTED;
}

STDMETHODIMP MySubstituteVirtualCameraFilter::GetNumberOfCapabilities(int *piCount, int *piSize)
{
    if (m_pOutputPin) {
        return m_pOutputPin->GetNumberOfCapabilities(piCount, piSize);
    }
    return E_UNEXPECTED;
}

STDMETHODIMP MySubstituteVirtualCameraFilter::GetStreamCaps(int iIndex, AM_MEDIA_TYPE **ppmt, BYTE *pSCC)
{
    if (m_pOutputPin) {
        return m_pOutputPin->GetStreamCaps(iIndex, ppmt, pSCC);
    }
    return E_UNEXPECTED;
}

// IKsPropertySet methods (for camera identification)
STDMETHODIMP MySubstituteVirtualCameraFilter::Set(REFGUID guidPropSet, DWORD dwPropID, LPVOID pInstanceData, DWORD cbInstanceData, LPVOID pPropData, DWORD cbPropData)
{
    return E_NOTIMPL;
}

STDMETHODIMP MySubstituteVirtualCameraFilter::Get(REFGUID guidPropSet, DWORD dwPropID, LPVOID pInstanceData, DWORD cbInstanceData, LPVOID pPropData, DWORD cbPropData, DWORD *pcbReturned)
{
    return E_NOTIMPL;
}

STDMETHODIMP MySubstituteVirtualCameraFilter::QuerySupported(REFGUID guidPropSet, DWORD dwPropID, DWORD *pTypeSupport)
{
    return E_NOTIMPL;
}

// Frame management methods
void MySubstituteVirtualCameraFilter::UpdateFrame(const Frame& frame)
{
    std::lock_guard<std::mutex> lock(m_frameMutex);
    m_latestFrame = frame;
}

Frame MySubstituteVirtualCameraFilter::GetLatestFrame()
{
    std::lock_guard<std::mutex> lock(m_frameMutex);
    return m_latestFrame;
}

HRESULT MySubstituteVirtualCameraFilter::GetPin(int n, IPin **ppPin)
{
    if (!ppPin) return E_POINTER;
    
    if (n == 0) {
        *ppPin = m_pOutputPin;
        m_pOutputPin->AddRef();
        return S_OK;
    }
    
    *ppPin = nullptr;
    return S_FALSE;
}

//=============================================================================
// MySubstituteOutputPin Implementation
//=============================================================================

MySubstituteOutputPin::MySubstituteOutputPin(MySubstituteVirtualCameraFilter* pFilter) :
    m_cRef(1),
    m_pFilter(pFilter),
    m_pConnectedPin(nullptr),
    m_bStreaming(false),
    m_hStreamingEvent(nullptr)
{
    InitializeCriticalSection(&m_PinLock);
    ZeroMemory(&m_mt, sizeof(AM_MEDIA_TYPE));
    
    // Create default media type (RGB24, 640x480)
    AM_MEDIA_TYPE* pTempMT = nullptr;
    if (SUCCEEDED(CreateMediaType(&pTempMT, 640, 480))) {
        CopyMediaType(&m_mt, pTempMT);
        DeleteMediaType(pTempMT);
    }
    
    m_hStreamingEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
}

MySubstituteOutputPin::~MySubstituteOutputPin()
{
    if (m_bStreaming) {
        m_bStreaming = false;
        if (m_StreamingThread.joinable()) {
            m_StreamingThread.join();
        }
    }
    
    if (m_hStreamingEvent) {
        CloseHandle(m_hStreamingEvent);
    }
    
    if (m_pConnectedPin) {
        m_pConnectedPin->Release();
    }
    
    DeleteMediaType(&m_mt);
    DeleteCriticalSection(&m_PinLock);
}

// IUnknown methods
STDMETHODIMP MySubstituteOutputPin::QueryInterface(REFIID riid, void **ppv)
{
    if (!ppv) return E_POINTER;
    
    if (riid == IID_IUnknown || riid == IID_IPin) {
        *ppv = static_cast<IPin*>(this);
    } else if (riid == IID_IAMStreamConfig) {
        *ppv = static_cast<IAMStreamConfig*>(this);
    } else {
        *ppv = nullptr;
        return E_NOINTERFACE;
    }
    
    AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG) MySubstituteOutputPin::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) MySubstituteOutputPin::Release()
{
    LONG cRef = InterlockedDecrement(&m_cRef);
    if (cRef == 0) {
        delete this;
    }
    return cRef;
}

// IPin methods
STDMETHODIMP MySubstituteOutputPin::Connect(IPin *pReceivePin, const AM_MEDIA_TYPE *pmt)
{
    if (!pReceivePin) return E_POINTER;
    
    Lock();
    
    if (m_pConnectedPin) {
        Unlock();
        return VFW_E_ALREADY_CONNECTED;
    }
    
    HRESULT hr = S_OK;
    
    if (pmt) {
        hr = CheckMediaType(pmt);
        if (SUCCEEDED(hr)) {
            hr = pReceivePin->ReceiveConnection(this, pmt);
        }
    } else {
        // Try default media type
        hr = pReceivePin->ReceiveConnection(this, &m_mt);
    }
    
    if (SUCCEEDED(hr)) {
        m_pConnectedPin = pReceivePin;
        m_pConnectedPin->AddRef();
    }
    
    Unlock();
    return hr;
}

STDMETHODIMP MySubstituteOutputPin::ReceiveConnection(IPin *pConnector, const AM_MEDIA_TYPE *pmt)
{
    return E_UNEXPECTED; // Output pins don't receive connections
}

STDMETHODIMP MySubstituteOutputPin::Disconnect()
{
    Lock();
    
    if (m_pConnectedPin) {
        m_pConnectedPin->Release();
        m_pConnectedPin = nullptr;
    }
    
    Unlock();
    return S_OK;
}

STDMETHODIMP MySubstituteOutputPin::ConnectedTo(IPin **pPin)
{
    if (!pPin) return E_POINTER;
    
    Lock();
    
    if (m_pConnectedPin) {
        *pPin = m_pConnectedPin;
        m_pConnectedPin->AddRef();
        Unlock();
        return S_OK;
    }
    
    Unlock();
    *pPin = nullptr;
    return VFW_E_NOT_CONNECTED;
}

STDMETHODIMP MySubstituteOutputPin::ConnectionMediaType(AM_MEDIA_TYPE *pmt)
{
    if (!pmt) return E_POINTER;
    
    Lock();
    
    if (!m_pConnectedPin) {
        Unlock();
        return VFW_E_NOT_CONNECTED;
    }
    
    CopyMediaType(pmt, &m_mt);
    Unlock();
    return S_OK;
}

STDMETHODIMP MySubstituteOutputPin::QueryPinInfo(PIN_INFO *pInfo)
{
    if (!pInfo) return E_POINTER;
    
    pInfo->pFilter = m_pFilter;
    m_pFilter->AddRef();
    pInfo->dir = PINDIR_OUTPUT;
    wcscpy_s(pInfo->achName, L"Output");
    
    return S_OK;
}

STDMETHODIMP MySubstituteOutputPin::QueryDirection(PIN_DIRECTION *pPinDir)
{
    if (!pPinDir) return E_POINTER;
    *pPinDir = PINDIR_OUTPUT;
    return S_OK;
}

STDMETHODIMP MySubstituteOutputPin::QueryId(LPWSTR *Id)
{
    if (!Id) return E_POINTER;
    
    *Id = (LPWSTR)CoTaskMemAlloc(sizeof(WCHAR) * 16);
    if (!*Id) return E_OUTOFMEMORY;
    
    wcscpy_s(*Id, 16, L"Output");
    return S_OK;
}

STDMETHODIMP MySubstituteOutputPin::QueryAccept(const AM_MEDIA_TYPE *pmt)
{
    return CheckMediaType(pmt);
}

STDMETHODIMP MySubstituteOutputPin::EnumMediaTypes(IEnumMediaTypes **ppEnum)
{
    // Simplified - return single media type
    return E_NOTIMPL;
}

STDMETHODIMP MySubstituteOutputPin::QueryInternalConnections(IPin **apPin, ULONG *nPin)
{
    return E_NOTIMPL;
}

STDMETHODIMP MySubstituteOutputPin::EndOfStream()
{
    return S_OK;
}

STDMETHODIMP MySubstituteOutputPin::BeginFlush()
{
    return S_OK;
}

STDMETHODIMP MySubstituteOutputPin::EndFlush()
{
    return S_OK;
}

STDMETHODIMP MySubstituteOutputPin::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
    return S_OK;
}

// IAMStreamConfig methods
STDMETHODIMP MySubstituteOutputPin::SetFormat(AM_MEDIA_TYPE *pmt)
{
    if (!pmt) return E_POINTER;
    
    Lock();
    HRESULT hr = CheckMediaType(pmt);
    if (SUCCEEDED(hr)) {
        DeleteMediaType(&m_mt);
        CopyMediaType(&m_mt, pmt);
    }
    Unlock();
    
    return hr;
}

STDMETHODIMP MySubstituteOutputPin::GetFormat(AM_MEDIA_TYPE **ppmt)
{
    if (!ppmt) return E_POINTER;
    
    Lock();
    *ppmt = (AM_MEDIA_TYPE*)CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE));
    if (*ppmt) {
        CopyMediaType(*ppmt, &m_mt);
    }
    Unlock();
    
    return *ppmt ? S_OK : E_OUTOFMEMORY;
}

STDMETHODIMP MySubstituteOutputPin::GetNumberOfCapabilities(int *piCount, int *piSize)
{
    if (!piCount || !piSize) return E_POINTER;
    
    *piCount = 1;
    *piSize = sizeof(VIDEO_STREAM_CONFIG_CAPS);
    return S_OK;
}

STDMETHODIMP MySubstituteOutputPin::GetStreamCaps(int iIndex, AM_MEDIA_TYPE **ppmt, BYTE *pSCC)
{
    if (iIndex != 0) return S_FALSE;
    if (!ppmt || !pSCC) return E_POINTER;
    
    CreateMediaType(ppmt, 640, 480);
    
    VIDEO_STREAM_CONFIG_CAPS* pvscc = (VIDEO_STREAM_CONFIG_CAPS*)pSCC;
    ZeroMemory(pvscc, sizeof(VIDEO_STREAM_CONFIG_CAPS));
    
    pvscc->guid = FORMAT_VideoInfo;
    pvscc->VideoStandard = AnalogVideo_None;
    pvscc->InputSize.cx = 640;
    pvscc->InputSize.cy = 480;
    pvscc->MinCroppingSize.cx = 320;
    pvscc->MinCroppingSize.cy = 240;
    pvscc->MaxCroppingSize.cx = 1920;
    pvscc->MaxCroppingSize.cy = 1080;
    pvscc->CropGranularityX = 1;
    pvscc->CropGranularityY = 1;
    pvscc->CropAlignX = 1;
    pvscc->CropAlignY = 1;
    pvscc->MinOutputSize.cx = 160;
    pvscc->MinOutputSize.cy = 120;
    pvscc->MaxOutputSize.cx = 1920;
    pvscc->MaxOutputSize.cy = 1080;
    pvscc->OutputGranularityX = 1;
    pvscc->OutputGranularityY = 1;
    pvscc->StretchTapsX = 1;
    pvscc->StretchTapsY = 1;
    pvscc->ShrinkTapsX = 1;
    pvscc->ShrinkTapsY = 1;
    pvscc->MinFrameInterval = FPS_30;
    pvscc->MaxFrameInterval = FPS_30;
    pvscc->MinBitsPerSecond = 320 * 240 * 24 * 30;
    pvscc->MaxBitsPerSecond = 1920 * 1080 * 24 * 30;
    
    return S_OK;
}

// Streaming control
HRESULT MySubstituteOutputPin::Active()
{
    if (!m_bStreaming && m_pConnectedPin) {
        m_bStreaming = true;
        m_StreamingThread = std::thread(&MySubstituteOutputPin::StreamingThreadProc, this);
    }
    return S_OK;
}

HRESULT MySubstituteOutputPin::Inactive()
{
    if (m_bStreaming) {
        m_bStreaming = false;
        SetEvent(m_hStreamingEvent);
        if (m_StreamingThread.joinable()) {
            m_StreamingThread.join();
        }
    }
    return S_OK;
}

HRESULT MySubstituteOutputPin::Run(REFERENCE_TIME tStart)
{
    return Active();
}

HRESULT MySubstituteOutputPin::Pause()
{
    return S_OK;
}

HRESULT MySubstituteOutputPin::Stop()
{
    return Inactive();
}

// Helper methods
HRESULT MySubstituteOutputPin::CheckMediaType(const AM_MEDIA_TYPE *pmt)
{
    if (!pmt) return E_POINTER;
    
    if (pmt->majortype != MEDIATYPE_Video) return VFW_E_TYPE_NOT_ACCEPTED;
    if (pmt->subtype != MEDIASUBTYPE_RGB24) return VFW_E_TYPE_NOT_ACCEPTED;
    if (pmt->formattype != FORMAT_VideoInfo) return VFW_E_TYPE_NOT_ACCEPTED;
    
    return S_OK;
}

void MySubstituteOutputPin::StreamingThreadProc()
{
    while (m_bStreaming) {
        if (WaitForSingleObject(m_hStreamingEvent, 33) == WAIT_OBJECT_0) { // ~30 FPS
            break;
        }
        
        DeliverSample();
    }
}

HRESULT MySubstituteOutputPin::DeliverSample()
{
    if (!m_pConnectedPin) return VFW_E_NOT_CONNECTED;
    
    // Get latest frame from filter
    Frame frame = m_pFilter->GetLatestFrame();
    if (frame.data.empty()) return S_OK;
    
    // Create media sample
    IMemAllocator* pAllocator = nullptr;
    IMediaSample* pSample = nullptr;
    
    // This is simplified - in a real implementation, you'd need to properly
    // negotiate allocators and create samples with correct timing
    
    return S_OK;
}

//=============================================================================
// MySubstitutePinEnum Implementation
//=============================================================================

MySubstitutePinEnum::MySubstitutePinEnum(MySubstituteVirtualCameraFilter* pFilter) :
    m_cRef(1),
    m_pFilter(pFilter),
    m_Position(0)
{
    m_pFilter->AddRef();
}

MySubstitutePinEnum::~MySubstitutePinEnum()
{
    m_pFilter->Release();
}

STDMETHODIMP MySubstitutePinEnum::QueryInterface(REFIID riid, void **ppv)
{
    if (!ppv) return E_POINTER;
    
    if (riid == IID_IUnknown || riid == IID_IEnumPins) {
        *ppv = this;
        AddRef();
        return S_OK;
    }
    
    *ppv = nullptr;
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) MySubstitutePinEnum::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) MySubstitutePinEnum::Release()
{
    LONG cRef = InterlockedDecrement(&m_cRef);
    if (cRef == 0) {
        delete this;
    }
    return cRef;
}

STDMETHODIMP MySubstitutePinEnum::Next(ULONG cPins, IPin **ppPins, ULONG *pcFetched)
{
    if (!ppPins) return E_POINTER;
    
    ULONG cFetched = 0;
    while (cFetched < cPins && m_Position < m_pFilter->GetPinCount()) {
        HRESULT hr = m_pFilter->GetPin(m_Position, &ppPins[cFetched]);
        if (SUCCEEDED(hr)) {
            cFetched++;
        }
        m_Position++;
    }
    
    if (pcFetched) {
        *pcFetched = cFetched;
    }
    
    return (cFetched == cPins) ? S_OK : S_FALSE;
}

STDMETHODIMP MySubstitutePinEnum::Skip(ULONG cPins)
{
    m_Position += cPins;
    return (m_Position <= m_pFilter->GetPinCount()) ? S_OK : S_FALSE;
}

STDMETHODIMP MySubstitutePinEnum::Reset()
{
    m_Position = 0;
    return S_OK;
}

STDMETHODIMP MySubstitutePinEnum::Clone(IEnumPins **ppEnum)
{
    if (!ppEnum) return E_POINTER;
    
    MySubstitutePinEnum* pEnum = new MySubstitutePinEnum(m_pFilter);
    pEnum->m_Position = m_Position;
    *ppEnum = pEnum;
    
    return S_OK;
}

//=============================================================================
// Helper functions
//=============================================================================

HRESULT CreateMediaType(AM_MEDIA_TYPE** ppmt, int width, int height)
{
    if (!ppmt) return E_POINTER;
    
    AM_MEDIA_TYPE* pmt = (AM_MEDIA_TYPE*)CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE));
    if (!pmt) return E_OUTOFMEMORY;
    
    ZeroMemory(pmt, sizeof(AM_MEDIA_TYPE));
    
    pmt->majortype = MEDIATYPE_Video;
    pmt->subtype = MEDIASUBTYPE_RGB24;
    pmt->bFixedSizeSamples = TRUE;
    pmt->bTemporalCompression = FALSE;
    pmt->formattype = FORMAT_VideoInfo;
    
    VIDEOINFOHEADER* pvih = (VIDEOINFOHEADER*)CoTaskMemAlloc(sizeof(VIDEOINFOHEADER));
    if (!pvih) {
        CoTaskMemFree(pmt);
        return E_OUTOFMEMORY;
    }
    
    ZeroMemory(pvih, sizeof(VIDEOINFOHEADER));
    pvih->AvgTimePerFrame = FPS_30;
    pvih->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    pvih->bmiHeader.biWidth = width;
    pvih->bmiHeader.biHeight = height;
    pvih->bmiHeader.biPlanes = 1;
    pvih->bmiHeader.biBitCount = 24;
    pvih->bmiHeader.biCompression = BI_RGB;
    pvih->bmiHeader.biSizeImage = width * height * 3;
    
    pmt->pbFormat = (BYTE*)pvih;
    pmt->cbFormat = sizeof(VIDEOINFOHEADER);
    pmt->lSampleSize = width * height * 3;
    
    *ppmt = pmt;
    return S_OK;
}

void DeleteMediaType(AM_MEDIA_TYPE* pmt)
{
    if (pmt) {
        if (pmt->pbFormat) {
            CoTaskMemFree(pmt->pbFormat);
        }
        CoTaskMemFree(pmt);
    }
}

HRESULT CopyMediaType(AM_MEDIA_TYPE* pmtTarget, const AM_MEDIA_TYPE* pmtSource)
{
    if (!pmtTarget || !pmtSource) return E_POINTER;
    
    *pmtTarget = *pmtSource;
    
    if (pmtSource->cbFormat > 0 && pmtSource->pbFormat) {
        pmtTarget->pbFormat = (BYTE*)CoTaskMemAlloc(pmtSource->cbFormat);
        if (!pmtTarget->pbFormat) return E_OUTOFMEMORY;
        
        memcpy(pmtTarget->pbFormat, pmtSource->pbFormat, pmtSource->cbFormat);
    }
    
    return S_OK;
}