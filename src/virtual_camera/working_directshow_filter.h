#pragma once
#include <windows.h>
#include <dshow.h>
#include <dvdmedia.h>

// We'll implement our own base classes since DirectShow base classes may not be available

// GUID for our virtual camera filter
// {B3F3A1C4-8F9E-4A2D-9B5C-7E6F8D4C9A3B}
DEFINE_GUID(CLSID_MySubstituteVirtualCameraFilter,
    0xB3F3A1C4, 0x8F9E, 0x4A2D, 0x9B, 0x5C, 0x7E, 0x6F, 0x8D, 0x4C, 0x9A, 0x3B);

// Forward declarations
class CMySubstituteVirtualCameraFilter;
class CMySubstituteOutputPin;

/**
 * DirectShow Source Filter for MySubstitute Virtual Camera
 * This creates a proper DirectShow source filter that appears as a camera device
 */
class CMySubstituteVirtualCameraFilter : public CSource
{
private:
    CMySubstituteVirtualCameraFilter(LPUNKNOWN lpunk, HRESULT* phr);

public:
    static CUnknown * WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT* phr);
    
    // Override base class methods
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv) override;
    
    // IUnknown methods
    DECLARE_IUNKNOWN

private:
    friend class CMySubstituteOutputPin;
};

/**
 * Output pin for the virtual camera filter
 * This provides the actual video data to applications
 */
class CMySubstituteOutputPin : public CSourceStream
{
public:
    CMySubstituteOutputPin(HRESULT* phr, CMySubstituteVirtualCameraFilter* pParent, LPCWSTR pPinName);
    ~CMySubstituteOutputPin();

    // Override CSourceStream methods
    HRESULT FillBuffer(IMediaSample* pms) override;
    HRESULT DecideBufferSize(IMemAllocator* pIMemAlloc, ALLOCATOR_PROPERTIES* pProperties) override;
    HRESULT CheckMediaType(const CMediaType* pMediaType) override;
    HRESULT GetMediaType(int iPosition, CMediaType* pmt) override;
    HRESULT SetMediaType(const CMediaType* pmt) override;
    
    // IAMStreamConfig interface for format negotiation
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv) override;
    
    // IAMStreamConfig methods
    STDMETHODIMP SetFormat(AM_MEDIA_TYPE* pmt);
    STDMETHODIMP GetFormat(AM_MEDIA_TYPE** ppmt);
    STDMETHODIMP GetNumberOfCapabilities(int* piCount, int* piSize);
    STDMETHODIMP GetStreamCaps(int iIndex, AM_MEDIA_TYPE** ppmt, BYTE* pSCC);

private:
    // Video format information
    VIDEOINFOHEADER m_VideoInfo;
    int m_iFrameNumber;
    REFERENCE_TIME m_rtSampleTime;
    
    // Frame generation
    HRESULT CreateTestFrame(BYTE* pData, long lDataLen);
    void GenerateColorBar(BYTE* pBuffer, int width, int height);
    
    // Format helpers
    HRESULT CreateMediaType(CMediaType* pmt, int width, int height, int fps);
    
    CCritSec m_cSharedState;
};