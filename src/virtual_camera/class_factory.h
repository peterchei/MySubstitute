#pragma once

#include <windows.h>
#include <unknwn.h>

/**
 * COM Class Factory for MySubstitute Virtual Camera
 * This enables the DirectShow filter to be created by the system
 */
class MySubstituteClassFactory : public IClassFactory
{
private:
    volatile LONG m_cRef;
    
public:
    MySubstituteClassFactory();
    virtual ~MySubstituteClassFactory();
    
    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();
    
    // IClassFactory methods
    STDMETHODIMP CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppv);
    STDMETHODIMP LockServer(BOOL fLock);
};

// DLL export functions for COM registration
extern "C" {
    STDAPI DllCanUnloadNow();
    STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv);
    STDAPI DllRegisterServer();
    STDAPI DllUnregisterServer();
}

// Global lock count for DLL unloading
extern LONG g_cServerLocks;