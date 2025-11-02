#include "directshow_filter_dll.h"
#include <dvdmedia.h>
#include <mmreg.h>

// Filter factory template - this registers our filter with DirectShow
CFactoryTemplate g_Templates[] = {
    {
        L"MySubstitute Virtual Camera",          // Name
        &CLSID_MySubstituteVirtualCameraFilter,  // CLSID
        CMySubstituteVirtualCameraFilter::CreateInstance, // Method to create an instance
        NULL,                                    // Initialization function
        &MEDIATYPE_Video                         // Category
    }
};
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

// DLL Entry Point
BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        break;
    }
    return TRUE;
}

// Standard DirectShow DLL exports
STDAPI DllCanUnloadNow(void)
{
    return (CBaseFilter::m_cRef == 0) ? S_OK : S_FALSE;
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    return CClassFactory::CreateInstance(rclsid, riid, ppv);
}

STDAPI DllRegisterServer(void)
{
    return AMovieDllRegisterServer2(TRUE);
}

STDAPI DllUnregisterServer(void)
{
    return AMovieDllRegisterServer2(FALSE);
}

//
// Filter Implementation
//

CMySubstituteVirtualCameraFilter::CMySubstituteVirtualCameraFilter(LPUNKNOWN lpunk, HRESULT *phr)
    : CSource(NAME("MySubstitute Virtual Camera"), lpunk, CLSID_MySubstituteVirtualCameraFilter)
{
    ASSERT(phr);
    
    // Create the one and only output pin
    m_paStreams = (CSourceStream **) new CMySubstituteOutputPin*[1];
    if (m_paStreams == NULL) {
        *phr = E_OUTOFMEMORY;
        return;
    }

    m_paStreams[0] = new CMySubstituteOutputPin(phr, this);
    if (m_paStreams[0] == NULL) {
        *phr = E_OUTOFMEMORY;
        return;
    }

    m_iPins = 1;
}

CMySubstituteVirtualCameraFilter::~CMySubstituteVirtualCameraFilter()
{
    delete m_paStreams[0];
    delete[] m_paStreams;
}

CUnknown * WINAPI CMySubstituteVirtualCameraFilter::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr)
{
    ASSERT(phr);
    
    CUnknown *punk = new CMySubstituteVirtualCameraFilter(lpunk, phr);
    if (punk == NULL) {
        if (phr)
            *phr = E_OUTOFMEMORY;
    }
    return punk;
}

STDMETHODIMP CMySubstituteVirtualCameraFilter::QueryInterface(REFIID riid, void **ppv)
{
    return CSource::QueryInterface(riid, ppv);
}

//
// Output Pin Implementation
//

CMySubstituteOutputPin::CMySubstituteOutputPin(HRESULT *phr, CSource *pFilter)
    : CSourceStream(NAME("MySubstitute Pin"), phr, pFilter, L"Out")
    , m_iFrameNumber(0)
    , m_iImageWidth(640)
    , m_iImageHeight(480)
{
    ASSERT(phr);
    
    // Initialize bitmap info
    CreateBitmap(m_iImageWidth, m_iImageHeight);
}

CMySubstituteOutputPin::~CMySubstituteOutputPin()
{
}

HRESULT CMySubstituteOutputPin::CreateBitmap(int width, int height)
{
    m_iImageWidth = width;
    m_iImageHeight = height;
    m_iImageSize = width * height * 3; // 24-bit RGB
    
    // Setup bitmap info
    ZeroMemory(&m_bmi, sizeof(m_bmi));
    m_bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    m_bmi.bmiHeader.biWidth = width;
    m_bmi.bmiHeader.biHeight = height;
    m_bmi.bmiHeader.biPlanes = 1;
    m_bmi.bmiHeader.biBitCount = 24;
    m_bmi.bmiHeader.biCompression = BI_RGB;
    m_bmi.bmiHeader.biSizeImage = m_iImageSize;

    return S_OK;
}

HRESULT CMySubstituteOutputPin::CheckMediaType(const CMediaType *pMediaType)
{
    CAutoLock cAutoLock(m_pFilter->pStateLock());
    
    if ((pMediaType->majortype != MEDIATYPE_Video) ||
        (pMediaType->subtype != MEDIASUBTYPE_RGB24) ||
        (pMediaType->formattype != FORMAT_VideoInfo)) {
        return E_INVALIDARG;
    }

    // Check the format looks reasonable
    VIDEOINFO *pvi = (VIDEOINFO *) pMediaType->pbFormat;
    if (pvi == NULL)
        return E_INVALIDARG;

    return S_OK;
}

HRESULT CMySubstituteOutputPin::GetMediaType(int iPosition, CMediaType *pmt)
{
    CAutoLock cAutoLock(m_pFilter->pStateLock());
    
    if (iPosition < 0)
        return E_INVALIDARG;
        
    if (iPosition > 0)
        return VFW_S_NO_MORE_ITEMS;

    // Allocate enough room for the VIDEOINFO and a color table
    VIDEOINFO *pvi = (VIDEOINFO*) pmt->AllocFormatBuffer(sizeof(VIDEOINFO));
    if (pvi == NULL)
        return E_OUTOFMEMORY;

    ZeroMemory(pvi, sizeof(VIDEOINFO));

    pvi->bmiHeader = m_bmi.bmiHeader;
    pvi->bmiHeader.biSizeImage = GetBitmapSize(&pvi->bmiHeader);

    // Frame rate: 25fps
    pvi->AvgTimePerFrame = FPS_25;

    SetRectEmpty(&(pvi->rcSource));
    SetRectEmpty(&(pvi->rcTarget));

    // Set the media type parameters
    pmt->majortype = MEDIATYPE_Video;
    pmt->subtype = MEDIASUBTYPE_RGB24;
    pmt->formattype = FORMAT_VideoInfo;
    pmt->bTemporalCompression = FALSE;
    pmt->bFixedSizeSamples = TRUE;
    pmt->lSampleSize = pvi->bmiHeader.biSizeImage;

    return NOERROR;
}

HRESULT CMySubstituteOutputPin::SetMediaType(const CMediaType *pmt)
{
    CAutoLock cAutoLock(m_pFilter->pStateLock());
    
    HRESULT hr = CSourceStream::SetMediaType(pmt);
    if (FAILED(hr))
        return hr;

    VIDEOINFO *pvi = (VIDEOINFO *) pmt->pbFormat;
    if (pvi == NULL)
        return E_UNEXPECTED;

    m_MediaType = *pmt;
    
    return NOERROR;
}

HRESULT CMySubstituteOutputPin::DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pProperties)
{
    CAutoLock cAutoLock(m_pFilter->pStateLock());
    
    ASSERT(pAlloc);
    ASSERT(pProperties);

    VIDEOINFO *pvi = (VIDEOINFO *) m_mt.Format();
    pProperties->cBuffers = 1;
    pProperties->cbBuffer = pvi->bmiHeader.biSizeImage;

    ASSERT(pProperties->cbBuffer);

    ALLOCATOR_PROPERTIES Actual;
    HRESULT hr = pAlloc->SetProperties(pProperties, &Actual);
    if (FAILED(hr))
        return hr;

    if (Actual.cbBuffer < pProperties->cbBuffer)
        return E_FAIL;

    ASSERT(Actual.cBuffers == 1);
    return NOERROR;
}

HRESULT CMySubstituteOutputPin::FillBuffer(IMediaSample *pms)
{
    BYTE *pData;
    long lDataLen;

    pms->GetPointer(&pData);
    lDataLen = pms->GetSize();

    // Generate a test frame
    GenerateTestFrame(pData, m_iFrameNumber);
    
    // Set the timestamps that will govern playback frame rate
    REFERENCE_TIME rtStart = m_iFrameNumber * FPS_25;
    REFERENCE_TIME rtStop = rtStart + FPS_25;
    
    pms->SetTime(&rtStart, &rtStop);
    pms->SetSyncPoint(TRUE);

    m_iFrameNumber++;
    
    return NOERROR;
}

void CMySubstituteOutputPin::GenerateTestFrame(BYTE* pData, int frameNumber)
{
    // Generate a simple test pattern with animation
    for (int y = 0; y < m_iImageHeight; y++) {
        for (int x = 0; x < m_iImageWidth; x++) {
            int index = ((m_iImageHeight - y - 1) * m_iImageWidth + x) * 3; // Flip Y for DirectShow
            
            // Create animated gradient pattern
            BYTE red = (BYTE)((x + frameNumber) % 256);
            BYTE green = (BYTE)((y + frameNumber/2) % 256);
            BYTE blue = (BYTE)((x + y + frameNumber) % 256);
            
            pData[index + 0] = blue;  // BGR format
            pData[index + 1] = green;
            pData[index + 2] = red;
        }
    }
}

HRESULT CMySubstituteOutputPin::OnThreadCreate(void)
{
    CAutoLock cAutoLock(&m_cSharedState);
    m_iFrameNumber = 0;
    return NOERROR;
}

HRESULT CMySubstituteOutputPin::OnThreadDestroy(void)
{
    return NOERROR;
}

HRESULT CMySubstituteOutputPin::OnThreadStartPlay(void)
{
    m_iFrameNumber = 0;
    return NOERROR;
}

STDMETHODIMP CMySubstituteOutputPin::Notify(IBaseFilter * pSender, Quality q)
{
    return E_NOTIMPL;
}