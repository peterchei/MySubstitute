#include "virtual_camera_directshow.h"
#include <strsafe.h>
#include <dvdmedia.h>
#include <mmreg.h>
#include <olectl.h>
#include <ks.h>
#include <ksmedia.h>
#include <iostream>
#include <cmath>

// DirectShow time units (100ns intervals)
#define UNITS 10000000

// Static frame rate (30 FPS)
const REFERENCE_TIME FPS_30 = UNITS / 30;

// Shared memory name (same as SimpleVirtualCamera)
const wchar_t* MySubstituteVirtualCameraFilter::SHARED_MEMORY_NAME = L"MySubstituteVirtualCameraFrames";

//=============================================================================
// MySubstituteVirtualCameraFilter Implementation
//=============================================================================

MySubstituteVirtualCameraFilter::MySubstituteVirtualCameraFilter() :
    m_cRef(1),
    m_State(State_Stopped),
    m_pClock(nullptr),
    m_pGraph(nullptr),
    m_pOutputPin(nullptr),
    m_sharedMemory(nullptr),
    m_sharedBuffer(nullptr)
{
    InitializeCriticalSection(&m_FilterLock);
    wcscpy_s(m_wszName, L"MySubstitute Virtual Camera");
    
    // Create output pin
    m_pOutputPin = new MySubstituteOutputPin(this);
    
    // Initialize shared memory for inter-process communication
    CreateSharedMemory();
}

MySubstituteVirtualCameraFilter::~MySubstituteVirtualCameraFilter()
{
    // Cleanup shared memory
    CleanupSharedMemory();
    
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
    // Handle AMPROPSETID_Pin property set (required for pin capabilities)
    if (guidPropSet == AMPROPSETID_Pin) {
        if (dwPropID == AMPROPERTY_PIN_CATEGORY) {
            if (pPropData && cbPropData >= sizeof(GUID)) {
                // Identify as capture pin
                *((GUID*)pPropData) = PIN_CATEGORY_CAPTURE;
                if (pcbReturned) *pcbReturned = sizeof(GUID);
                return S_OK;
            }
            return E_INVALIDARG;
        }
    }
    
    return E_PROP_SET_UNSUPPORTED;
}

STDMETHODIMP MySubstituteVirtualCameraFilter::QuerySupported(REFGUID guidPropSet, DWORD dwPropID, DWORD *pTypeSupport)
{
    if (guidPropSet == AMPROPSETID_Pin) {
        if (dwPropID == AMPROPERTY_PIN_CATEGORY) {
            if (pTypeSupport) {
                *pTypeSupport = KSPROPERTY_SUPPORT_GET;
            }
            return S_OK;
        }
    }
    
    return E_PROP_SET_UNSUPPORTED;
}

// Frame management methods
void MySubstituteVirtualCameraFilter::UpdateFrame(const Frame& frame)
{
    std::lock_guard<std::mutex> lock(m_frameMutex);
    m_latestFrame = frame;
}

Frame MySubstituteVirtualCameraFilter::GetLatestFrame()
{
    // Try to read from shared memory first (from main process)
    Frame sharedFrame = ReadFrameFromSharedMemory();
    if (!sharedFrame.data.empty()) {
        return sharedFrame;
    }
    
    // Fallback to local frame data
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
    m_hStreamingEvent(nullptr),
    m_pMemAllocator(nullptr),
    m_sampleCount(0)
{
    InitializeCriticalSection(&m_PinLock);
    ZeroMemory(&m_mt, sizeof(AM_MEDIA_TYPE));
    
    // Initialize basic media type structure without COM allocations
    // Full initialization will happen lazily when needed
    m_mt.majortype = MEDIATYPE_Video;
    m_mt.subtype = MEDIASUBTYPE_RGB24;
    m_mt.formattype = FORMAT_VideoInfo;
    m_mt.bFixedSizeSamples = TRUE;
    m_mt.bTemporalCompression = FALSE;
    m_mt.lSampleSize = 640 * 480 * 3;
    
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
    
    if (m_pMemAllocator) {
        m_pMemAllocator->Release();
    }
    
    // Clean up media type format data if allocated
    if (m_mt.pbFormat) {
        CoTaskMemFree(m_mt.pbFormat);
    }
    
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
    } else if (riid == IID_IKsPropertySet) {
        *ppv = static_cast<IKsPropertySet*>(this);
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
        
        // Negotiate memory allocator
        IMemInputPin* pMemInputPin = nullptr;
        hr = pReceivePin->QueryInterface(IID_IMemInputPin, (void**)&pMemInputPin);
        if (SUCCEEDED(hr)) {
            hr = pMemInputPin->GetAllocator(&m_pMemAllocator);
            if (FAILED(hr)) {
                // Create our own allocator
                hr = CoCreateInstance(CLSID_MemoryAllocator, nullptr, CLSCTX_INPROC_SERVER,
                                    IID_IMemAllocator, (void**)&m_pMemAllocator);
            }
            
            if (SUCCEEDED(hr) && m_pMemAllocator) {
                // Set allocator properties
                ALLOCATOR_PROPERTIES props;
                props.cBuffers = 3;
                props.cbBuffer = 640 * 480 * 3; // RGB24
                props.cbAlign = 1;
                props.cbPrefix = 0;
                
                ALLOCATOR_PROPERTIES actualProps;
                hr = m_pMemAllocator->SetProperties(&props, &actualProps);
                if (SUCCEEDED(hr)) {
                    hr = pMemInputPin->NotifyAllocator(m_pMemAllocator, FALSE);
                    if (SUCCEEDED(hr)) {
                        // CRITICAL: Commit the allocator so it can provide buffers
                        hr = m_pMemAllocator->Commit();
                    }
                }
            }
            
            pMemInputPin->Release();
        }
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
    
    // Stop streaming first
    Inactive();
    
    if (m_pMemAllocator) {
        m_pMemAllocator->Decommit();
        m_pMemAllocator->Release();
        m_pMemAllocator = nullptr;
    }
    
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
    if (!ppEnum) return E_POINTER;
    
    // Create our media type enumerator
    return CreateMediaTypeEnumerator(ppEnum);
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
        // Clean up existing format data
        if (m_mt.pbFormat) {
            CoTaskMemFree(m_mt.pbFormat);
            m_mt.pbFormat = nullptr;
        }
        // Copy new media type
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

// IKsPropertySet methods for pin
STDMETHODIMP MySubstituteOutputPin::Set(REFGUID guidPropSet, DWORD dwPropID, LPVOID pInstanceData, DWORD cbInstanceData, LPVOID pPropData, DWORD cbPropData)
{
    return E_NOTIMPL;
}

STDMETHODIMP MySubstituteOutputPin::Get(REFGUID guidPropSet, DWORD dwPropID, LPVOID pInstanceData, DWORD cbInstanceData, LPVOID pPropData, DWORD cbPropData, DWORD *pcbReturned)
{
    // Handle AMPROPSETID_Pin property set (required for pin identification)
    if (guidPropSet == AMPROPSETID_Pin) {
        if (dwPropID == AMPROPERTY_PIN_CATEGORY) {
            if (pPropData && cbPropData >= sizeof(GUID)) {
                // Identify as capture output pin
                *((GUID*)pPropData) = PIN_CATEGORY_CAPTURE;
                if (pcbReturned) *pcbReturned = sizeof(GUID);
                return S_OK;
            }
            return E_INVALIDARG;
        }
    }
    
    return E_PROP_SET_UNSUPPORTED;
}

STDMETHODIMP MySubstituteOutputPin::QuerySupported(REFGUID guidPropSet, DWORD dwPropID, DWORD *pTypeSupport)
{
    if (guidPropSet == AMPROPSETID_Pin) {
        if (dwPropID == AMPROPERTY_PIN_CATEGORY) {
            if (pTypeSupport) {
                *pTypeSupport = KSPROPERTY_SUPPORT_GET;
            }
            return S_OK;
        }
    }
    
    return E_PROP_SET_UNSUPPORTED;
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

HRESULT MySubstituteOutputPin::GetMediaType(int iPosition, AM_MEDIA_TYPE *pmt)
{
    if (!pmt) return E_POINTER;
    if (iPosition < 0) return E_INVALIDARG;
    if (iPosition > 0) return VFW_S_NO_MORE_ITEMS;  // We only support one media type
    
    // Initialize the media type
    ZeroMemory(pmt, sizeof(AM_MEDIA_TYPE));
    
    // Set up RGB24 video format 640x480
    pmt->majortype = MEDIATYPE_Video;
    pmt->subtype = MEDIASUBTYPE_RGB24;
    pmt->formattype = FORMAT_VideoInfo;
    pmt->bFixedSizeSamples = TRUE;
    pmt->bTemporalCompression = FALSE;
    pmt->lSampleSize = 640 * 480 * 3;  // RGB24 = 3 bytes per pixel
    
    // Allocate and fill VIDEOINFOHEADER - this now happens safely after COM is initialized
    VIDEOINFOHEADER* pvi = (VIDEOINFOHEADER*)CoTaskMemAlloc(sizeof(VIDEOINFOHEADER));
    if (!pvi) return E_OUTOFMEMORY;
    
    ZeroMemory(pvi, sizeof(VIDEOINFOHEADER));
    pvi->AvgTimePerFrame = FPS_30;  // 30 FPS
    pvi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    pvi->bmiHeader.biWidth = 640;
    pvi->bmiHeader.biHeight = 480;
    pvi->bmiHeader.biPlanes = 1;
    pvi->bmiHeader.biBitCount = 24;
    pvi->bmiHeader.biCompression = BI_RGB;
    pvi->bmiHeader.biSizeImage = 640 * 480 * 3;
    
    pmt->pbFormat = (BYTE*)pvi;
    pmt->cbFormat = sizeof(VIDEOINFOHEADER);
    
    return S_OK;
}

HRESULT MySubstituteOutputPin::CreateMediaTypeEnumerator(IEnumMediaTypes **ppEnum)
{
    if (!ppEnum) return E_POINTER;
    
    // Create a simple media type enumerator that returns our single media type
    *ppEnum = new MySubstituteMediaTypeEnum(this);
    if (!*ppEnum) return E_OUTOFMEMORY;
    
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
    if (!m_pMemAllocator) return VFW_E_NO_ALLOCATOR;
    
    // Get latest frame from filter  
    Frame frame = m_pFilter->GetLatestFrame();
    
    // If no frame data, generate test pattern
    if (frame.data.empty()) {
        frame = GenerateTestFrame();
    }
    
    // Get a media sample from allocator
    IMediaSample* pSample = nullptr;
    HRESULT hr = m_pMemAllocator->GetBuffer(&pSample, nullptr, nullptr, 0);
    if (FAILED(hr)) return hr;
    
    // Get sample buffer
    BYTE* pBuffer = nullptr;
    hr = pSample->GetPointer(&pBuffer);
    if (FAILED(hr)) {
        pSample->Release();
        return hr;
    }
    
    // Get buffer size 
    long bufferSize = pSample->GetSize();
    
    // Copy frame data to sample buffer
#if defined(HAVE_OPENCV) && (HAVE_OPENCV == 1)
    if (frame.width > 0 && frame.height > 0 && !frame.data.empty()) {
        // Convert OpenCV Mat to RGB24 if needed
        cv::Mat rgbFrame;
        if (frame.data.channels() == 3) {
            cv::cvtColor(frame.data, rgbFrame, cv::COLOR_BGR2RGB);
        } else {
            rgbFrame = frame.data;
        }
        
        // Flip vertically for DirectShow (bottom-up DIB format)
        cv::Mat flippedFrame;
        cv::flip(rgbFrame, flippedFrame, 0);
        
        // Copy to buffer
        int frameSize = flippedFrame.cols * flippedFrame.rows * flippedFrame.channels();
        if (frameSize <= bufferSize) {
            memcpy(pBuffer, flippedFrame.data, frameSize);
            pSample->SetActualDataLength(frameSize);
        } else {
            // Frame too big, fill with test pattern
            GenerateTestFrameData(pBuffer, bufferSize);
            pSample->SetActualDataLength(bufferSize);
        }
    } else {
        // No frame data, generate test pattern
        GenerateTestFrameData(pBuffer, bufferSize);
        pSample->SetActualDataLength(bufferSize);
    }
#else
    // When OpenCV is not available, always generate test pattern
    GenerateTestFrameData(pBuffer, bufferSize);
    pSample->SetActualDataLength(bufferSize);
#endif
    
    // Set sample timestamps (30 FPS)
    REFERENCE_TIME startTime = m_sampleCount * 333333; // 100ns units
    REFERENCE_TIME endTime = startTime + 333333;
    pSample->SetTime(&startTime, &endTime);
    
    // Mark as sync point (key frame)
    pSample->SetSyncPoint(TRUE);
    
    // Deliver the sample to IMemInputPin
    IMemInputPin* pMemInputPin = nullptr;
    hr = m_pConnectedPin->QueryInterface(IID_IMemInputPin, (void**)&pMemInputPin);
    if (SUCCEEDED(hr)) {
        hr = pMemInputPin->Receive(pSample);
        pMemInputPin->Release();
    }
    
    // Cleanup
    pSample->Release();
    m_sampleCount++;
    
    return hr;
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
// Helper methods for MySubstituteOutputPin
//=============================================================================

Frame MySubstituteOutputPin::GenerateTestFrame()
{
    Frame testFrame;
    testFrame.width = 640;
    testFrame.height = 480;
    testFrame.channels = 3;
    
#if HAVE_OPENCV
    // Create a test pattern (colorful gradient)
    cv::Mat testMat(480, 640, CV_8UC3);
    for (int y = 0; y < 480; y++) {
        for (int x = 0; x < 640; x++) {
            int red = (x * 255) / 640;
            int green = (y * 255) / 480;
            int blue = ((x + y) * 255) / (640 + 480);
            
            testMat.at<cv::Vec3b>(y, x) = cv::Vec3b(blue, green, red);
        }
    }
    
    // Add text overlay
    cv::putText(testMat, "MySubstitute Virtual Camera", 
                cv::Point(50, 240), cv::FONT_HERSHEY_SIMPLEX, 
                1.0, cv::Scalar(255, 255, 255), 2);
    
    testFrame.data = testMat;
#endif
    
    return testFrame;
}

void MySubstituteOutputPin::GenerateTestFrameData(BYTE* pBuffer, long bufferSize)
{
    if (!pBuffer || bufferSize < (640 * 480 * 3)) return;
    
    // Generate animated test pattern with moving elements
    static DWORD animationOffset = 0;
    animationOffset += 5; // Animation speed
    
    DWORD timeOffset = GetTickCount() / 100; // Smooth time-based animation
    
    for (int y = 0; y < 480; y++) {
        for (int x = 0; x < 640; x++) {
            int index = (y * 640 + x) * 3;
            if (index + 2 < bufferSize) {
                // Create animated gradient with moving wave patterns
                int wave1 = (int)(127 * sin((x + timeOffset * 2) * 0.01) + 128);
                int wave2 = (int)(127 * sin((y + timeOffset * 3) * 0.01) + 128);
                int wave3 = (int)(127 * sin((x + y + timeOffset * 4) * 0.005) + 128);
                
                // Add moving color bands
                int band = ((x + y + animationOffset) / 20) % 3;
                
                switch(band) {
                    case 0: // Red band
                        pBuffer[index + 0] = (BYTE)(wave1);  // Red
                        pBuffer[index + 1] = (BYTE)(wave2 / 4);  // Green
                        pBuffer[index + 2] = (BYTE)(wave3 / 4);  // Blue
                        break;
                    case 1: // Green band  
                        pBuffer[index + 0] = (BYTE)(wave1 / 4);  // Red
                        pBuffer[index + 1] = (BYTE)(wave2);      // Green
                        pBuffer[index + 2] = (BYTE)(wave3 / 4);  // Blue
                        break;
                    case 2: // Blue band
                        pBuffer[index + 0] = (BYTE)(wave1 / 4);  // Red
                        pBuffer[index + 1] = (BYTE)(wave2 / 4);  // Green  
                        pBuffer[index + 2] = (BYTE)(wave3);      // Blue
                        break;
                }
                
                // Add frame counter in top-left corner
                if (x < 100 && y < 30) {
                    if ((x / 10 + y / 10) % 2 == (timeOffset / 10) % 2) {
                        pBuffer[index + 0] = 255; // White text area
                        pBuffer[index + 1] = 255;
                        pBuffer[index + 2] = 255;
                    }
                }
            }
        }
    }
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

//
// MySubstituteMediaTypeEnum Implementation
//
MySubstituteMediaTypeEnum::MySubstituteMediaTypeEnum(MySubstituteOutputPin* pPin) 
    : m_cRef(1), m_pPin(pPin), m_Position(0) 
{
    if (m_pPin) m_pPin->AddRef();
}

MySubstituteMediaTypeEnum::~MySubstituteMediaTypeEnum() 
{
    if (m_pPin) m_pPin->Release();
}

STDMETHODIMP MySubstituteMediaTypeEnum::QueryInterface(REFIID riid, void **ppv) 
{
    if (riid == IID_IUnknown || riid == IID_IEnumMediaTypes) {
        *ppv = this;
        AddRef();
        return S_OK;
    }
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) MySubstituteMediaTypeEnum::AddRef() 
{ 
    return InterlockedIncrement(&m_cRef); 
}

STDMETHODIMP_(ULONG) MySubstituteMediaTypeEnum::Release() 
{
    LONG cRef = InterlockedDecrement(&m_cRef);
    if (cRef == 0) delete this;
    return cRef;
}

STDMETHODIMP MySubstituteMediaTypeEnum::Next(ULONG cMediaTypes, AM_MEDIA_TYPE **ppMediaTypes, ULONG *pcFetched) 
{
    if (!ppMediaTypes) return E_POINTER;
    if (cMediaTypes == 0) return S_OK;
    
    ULONG fetched = 0;
    
    if (m_Position == 0 && cMediaTypes > 0) {
        AM_MEDIA_TYPE* pmt = (AM_MEDIA_TYPE*)CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE));
        if (!pmt) return E_OUTOFMEMORY;
        
        HRESULT hr = m_pPin->GetMediaType(0, pmt);
        if (SUCCEEDED(hr)) {
            ppMediaTypes[0] = pmt;
            fetched = 1;
            m_Position = 1;
        } else {
            CoTaskMemFree(pmt);
            return hr;
        }
    }
    
    if (pcFetched) *pcFetched = fetched;
    return (fetched == cMediaTypes) ? S_OK : S_FALSE;
}

STDMETHODIMP MySubstituteMediaTypeEnum::Skip(ULONG cMediaTypes) 
{
    m_Position += cMediaTypes;
    return (m_Position <= 1) ? S_OK : S_FALSE;
}

STDMETHODIMP MySubstituteMediaTypeEnum::Reset() 
{
    m_Position = 0;
    return S_OK;
}

STDMETHODIMP MySubstituteMediaTypeEnum::Clone(IEnumMediaTypes **ppEnum) 
{
    if (!ppEnum) return E_POINTER;
    *ppEnum = new MySubstituteMediaTypeEnum(m_pPin);
    (*ppEnum)->Skip(m_Position);
    return S_OK;
}

//=============================================================================
// MySubstituteVirtualCameraFilter - Shared Memory Helper Methods
//=============================================================================

bool MySubstituteVirtualCameraFilter::CreateSharedMemory()
{
    // Try to open existing shared memory (created by main process)
    m_sharedMemory = OpenFileMappingW(
        FILE_MAP_ALL_ACCESS,
        FALSE,
        SHARED_MEMORY_NAME
    );
    
    if (m_sharedMemory == nullptr) {
        // If no existing memory, create it ourselves (fallback)
        m_sharedMemory = CreateFileMappingW(
            INVALID_HANDLE_VALUE,
            nullptr,
            PAGE_READWRITE,
            0,
            SHARED_BUFFER_SIZE,
            SHARED_MEMORY_NAME
        );
    }
    
    if (m_sharedMemory == nullptr) {
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
        return false;
    }
    
    return true;
}

void MySubstituteVirtualCameraFilter::CleanupSharedMemory()
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

Frame MySubstituteVirtualCameraFilter::ReadFrameFromSharedMemory()
{
    Frame frame;
    
    if (!m_sharedBuffer) {
        return frame; // Empty frame
    }
    
#if defined(HAVE_OPENCV) && (HAVE_OPENCV == 1)
    try {
        // Shared memory contains RGB24 data (640x480x3)
        int width = 640;
        int height = 480;
        int channels = 3;
        
        // Create OpenCV Mat from shared memory data
        cv::Mat rgbMat(height, width, CV_8UC3, m_sharedBuffer);
        
        // Convert RGB to BGR (OpenCV format)
        cv::Mat bgrMat;
        cv::cvtColor(rgbMat, bgrMat, cv::COLOR_RGB2BGR);
        
        // Create frame
        frame.width = width;
        frame.height = height;
        frame.channels = channels;
        frame.data = bgrMat.clone(); // Important: clone to avoid shared memory issues
        
    } catch (...) {
        // If OpenCV fails, return empty frame
        frame = Frame();
    }
#endif
    
    return frame;
}