#pragma once

#include <windows.h>
#include <dshow.h>
#include <dvdmedia.h>
#include <initguid.h>
#include <memory>
#include <mutex>
#include <thread>
#include <atomic>
#include "../capture/frame.h"

// MySubstitute Virtual Camera CLSID
// {B3F3A1C4-8F9E-4A2D-9B5C-7E6F8D4C9A3B}
DEFINE_GUID(CLSID_MySubstituteVirtualCamera,
    0xb3f3a1c4, 0x8f9e, 0x4a2d, 0x9b, 0x5c, 0x7e, 0x6f, 0x8d, 0x4c, 0x9a, 0x3b);

// Forward declarations
class MySubstituteVirtualCameraFilter;
class MySubstituteOutputPin;

/**
 * Complete DirectShow Virtual Camera Filter Implementation
 * This creates a real system camera device visible to all applications
 */
class MySubstituteVirtualCameraFilter : 
    public IBaseFilter, 
    public IAMStreamConfig,
    public IKsPropertySet
{
private:
    volatile LONG m_cRef;
    CRITICAL_SECTION m_FilterLock;
    FILTER_STATE m_State;
    IReferenceClock* m_pClock;
    IFilterGraph* m_pGraph;
    WCHAR m_wszName[128];
    
    MySubstituteOutputPin* m_pOutputPin;
    Frame m_latestFrame;
    std::mutex m_frameMutex;
    
    MySubstituteVirtualCameraFilter();
    
public:
    virtual ~MySubstituteVirtualCameraFilter();
    
    static MySubstituteVirtualCameraFilter* CreateInstance();
    
    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();
    
    // IPersist method
    STDMETHODIMP GetClassID(CLSID *pClsID);
    
    // IMediaFilter methods
    STDMETHODIMP Stop();
    STDMETHODIMP Pause();
    STDMETHODIMP Run(REFERENCE_TIME tStart);
    STDMETHODIMP GetState(DWORD dwMilliSecsTimeout, FILTER_STATE *State);
    STDMETHODIMP SetSyncSource(IReferenceClock *pClock);
    STDMETHODIMP GetSyncSource(IReferenceClock **pClock);
    
    // IBaseFilter methods
    STDMETHODIMP EnumPins(IEnumPins **ppEnum);
    STDMETHODIMP FindPin(LPCWSTR Id, IPin **ppPin);
    STDMETHODIMP QueryFilterInfo(FILTER_INFO *pInfo);
    STDMETHODIMP JoinFilterGraph(IFilterGraph *pGraph, LPCWSTR pName);
    STDMETHODIMP QueryVendorInfo(LPWSTR *pVendorInfo);
    
    // IAMStreamConfig methods
    STDMETHODIMP SetFormat(AM_MEDIA_TYPE *pmt);
    STDMETHODIMP GetFormat(AM_MEDIA_TYPE **ppmt);
    STDMETHODIMP GetNumberOfCapabilities(int *piCount, int *piSize);
    STDMETHODIMP GetStreamCaps(int iIndex, AM_MEDIA_TYPE **ppmt, BYTE *pSCC);
    
    // IKsPropertySet methods (for camera identification)
    STDMETHODIMP Set(REFGUID guidPropSet, DWORD dwPropID, LPVOID pInstanceData, DWORD cbInstanceData, LPVOID pPropData, DWORD cbPropData);
    STDMETHODIMP Get(REFGUID guidPropSet, DWORD dwPropID, LPVOID pInstanceData, DWORD cbInstanceData, LPVOID pPropData, DWORD cbPropData, DWORD *pcbReturned);
    STDMETHODIMP QuerySupported(REFGUID guidPropSet, DWORD dwPropID, DWORD *pTypeSupport);
    
    // Frame management methods
    void UpdateFrame(const Frame& frame);
    Frame GetLatestFrame();
    
    // Internal methods
    HRESULT GetPin(int n, IPin **ppPin);
    int GetPinCount() { return 1; }
    
    // Lock methods
    void Lock() { EnterCriticalSection(&m_FilterLock); }
    void Unlock() { LeaveCriticalSection(&m_FilterLock); }
};

/**
 * DirectShow Output Pin Implementation
 * Delivers processed video frames to applications
 */
class MySubstituteOutputPin : public IPin, public IAMStreamConfig
{
private:
    volatile LONG m_cRef;
    MySubstituteVirtualCameraFilter* m_pFilter;
    IPin* m_pConnectedPin;
    AM_MEDIA_TYPE m_mt;
    CRITICAL_SECTION m_PinLock;
    
    // Streaming thread
    std::thread m_StreamingThread;
    std::atomic<bool> m_bStreaming;
    HANDLE m_hStreamingEvent;
    
public:
    MySubstituteOutputPin(MySubstituteVirtualCameraFilter* pFilter);
    virtual ~MySubstituteOutputPin();
    
    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();
    
    // IPin methods
    STDMETHODIMP Connect(IPin *pReceivePin, const AM_MEDIA_TYPE *pmt);
    STDMETHODIMP ReceiveConnection(IPin *pConnector, const AM_MEDIA_TYPE *pmt);
    STDMETHODIMP Disconnect();
    STDMETHODIMP ConnectedTo(IPin **pPin);
    STDMETHODIMP ConnectionMediaType(AM_MEDIA_TYPE *pmt);
    STDMETHODIMP QueryPinInfo(PIN_INFO *pInfo);
    STDMETHODIMP QueryDirection(PIN_DIRECTION *pPinDir);
    STDMETHODIMP QueryId(LPWSTR *Id);
    STDMETHODIMP QueryAccept(const AM_MEDIA_TYPE *pmt);
    STDMETHODIMP EnumMediaTypes(IEnumMediaTypes **ppEnum);
    STDMETHODIMP QueryInternalConnections(IPin **apPin, ULONG *nPin);
    STDMETHODIMP EndOfStream();
    STDMETHODIMP BeginFlush();
    STDMETHODIMP EndFlush();
    STDMETHODIMP NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);
    
    // IAMStreamConfig methods
    STDMETHODIMP SetFormat(AM_MEDIA_TYPE *pmt);
    STDMETHODIMP GetFormat(AM_MEDIA_TYPE **ppmt);
    STDMETHODIMP GetNumberOfCapabilities(int *piCount, int *piSize);
    STDMETHODIMP GetStreamCaps(int iIndex, AM_MEDIA_TYPE **ppmt, BYTE *pSCC);
    
    // Streaming control
    HRESULT Active();
    HRESULT Inactive();
    HRESULT Run(REFERENCE_TIME tStart);
    HRESULT Pause();
    HRESULT Stop();
    
private:
    // Helper methods
    HRESULT CheckMediaType(const AM_MEDIA_TYPE *pmt);
    HRESULT GetMediaType(int iPosition, AM_MEDIA_TYPE *pmt);
    void StreamingThreadProc();
    HRESULT DeliverSample();
    
    // Lock methods
    void Lock() { EnterCriticalSection(&m_PinLock); }
    void Unlock() { LeaveCriticalSection(&m_PinLock); }
};

/**
 * Pin Enumerator for DirectShow
 */
class MySubstitutePinEnum : public IEnumPins
{
private:
    volatile LONG m_cRef;
    MySubstituteVirtualCameraFilter* m_pFilter;
    int m_Position;
    
public:
    MySubstitutePinEnum(MySubstituteVirtualCameraFilter* pFilter);
    virtual ~MySubstitutePinEnum();
    
    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();
    
    // IEnumPins methods
    STDMETHODIMP Next(ULONG cPins, IPin **ppPins, ULONG *pcFetched);
    STDMETHODIMP Skip(ULONG cPins);
    STDMETHODIMP Reset();
    STDMETHODIMP Clone(IEnumPins **ppEnum);
};

// Helper functions for media type management
HRESULT CreateMediaType(AM_MEDIA_TYPE** ppmt, int width = 640, int height = 480);
void DeleteMediaType(AM_MEDIA_TYPE* pmt);
HRESULT CopyMediaType(AM_MEDIA_TYPE* pmtTarget, const AM_MEDIA_TYPE* pmtSource);