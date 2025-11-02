#pragma once

#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mferror.h>
#include <mfreadwrite.h>
#include <wmcodecdsp.h>
#include <assert.h>
#include <strsafe.h>

// Our virtual camera GUID
// {A3FCE0F0-3824-4902-B1D1-3930B4F3A5B7}
DEFINE_GUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID_MYSUBSTITUTE,
    0xa3fce0f0, 0x3824, 0x4902, 0xb1, 0xd1, 0x39, 0x30, 0xb4, 0xf3, 0xa5, 0xb7);

class MediaFoundationVirtualCamera
{
public:
    static HRESULT RegisterVirtualCamera();
    static HRESULT UnregisterVirtualCamera();
    static HRESULT TestCameraVisibility();
    static bool IsRegistered();
    static void ShowStatus();

private:
    static HRESULT CreateRegistryEntries();
    static HRESULT DeleteRegistryEntries();
    static HRESULT EnumerateVideoDevices();
    static const wchar_t* GetCameraName() { return L"MySubstitute Virtual Camera"; }
    static const wchar_t* GetCameraDescription() { return L"MySubstitute AI-Enhanced Virtual Camera"; }
};