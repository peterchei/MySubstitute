#pragma once

#include <windows.h>
#include <streams.h>
#include <initguid.h>
#include <dvdmedia.h>

// Define our filter GUID - this will identify our virtual camera
// {E23B9B10-5D01-11D0-BD3B-00A0C911CE86}
DEFINE_GUID(CLSID_MySubstituteVirtualCameraFilter, 
    0xe23b9b10, 0x5d01, 0x11d0, 0xbd, 0x3b, 0x00, 0xa0, 0xc9, 0x11, 0xce, 0x86);

// Forward declarations
class CMySubstituteVirtualCameraFilter;
class CMySubstituteOutputPin;

// Template instantiation for our filter factory
extern CFactoryTemplate g_Templates[];
extern int g_cTemplates;

//
// MySubstitute Virtual Camera Filter - Main DirectShow Filter
//
class CMySubstituteVirtualCameraFilter : public CSource
{
private:
    // Private constructor - use CreateInstance instead
    CMySubstituteVirtualCameraFilter(LPUNKNOWN lpunk, HRESULT *phr);

public:
    virtual ~CMySubstituteVirtualCameraFilter();
    
    // Factory method to create instances
    static CUnknown * WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);

    // DirectShow filter methods
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    
    // Filter name for debugging
    LPWSTR GetFilterName() { return L"MySubstitute Virtual Camera"; }
};

//
// MySubstitute Output Pin - Provides video data
//
class CMySubstituteOutputPin : public CSourceStream
{
private:
    int m_iFrameNumber;
    const REFERENCE_TIME FPS_25 = UNITS / 25;  // 25 FPS
    
    BITMAPINFO m_bmi;
    int m_iImageWidth;
    int m_iImageHeight;
    int m_iImageSize;
    
    CCritSec m_cSharedState;    // Protects our internal state
    CMediaType m_MediaType;     // Current media type
    
public:
    CMySubstituteOutputPin(HRESULT *phr, CSource *pFilter);
    virtual ~CMySubstituteOutputPin();

    // Override these to provide data
    HRESULT FillBuffer(IMediaSample *pSample);
    HRESULT DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pProperties);
    HRESULT CheckMediaType(const CMediaType *pMediaType);
    HRESULT GetMediaType(int iPosition, CMediaType *pmt);
    HRESULT SetMediaType(const CMediaType *pmt);
    HRESULT OnThreadCreate(void);
    HRESULT OnThreadDestroy(void);
    HRESULT OnThreadStartPlay(void);

    // Quality control
    STDMETHODIMP Notify(IBaseFilter * pSender, Quality q);

    // Helper methods
    HRESULT CreateBitmap(int width, int height);
    void GenerateTestFrame(BYTE* pData, int frameNumber);
};

//
// DLL Export Functions
//
extern "C" {
    BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved);
    STDAPI DllCanUnloadNow(void);
    STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv);
    STDAPI DllRegisterServer(void);
    STDAPI DllUnregisterServer(void);
}