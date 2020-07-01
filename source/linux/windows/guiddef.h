#pragma once

#include "linux/windows/wincompat.h"

typedef int GUID;
typedef GUID *LPGUID;
typedef GUID REFCLSID;
typedef GUID REFIID;

#define CLSCTX_INPROC 0
#define CLSID_SystemClock 1
#define IID_IReferenceClock 2

struct IUnknown
{
  HRESULT QueryInterface(int riid, void **ppvObject);
  HRESULT Release();
};
typedef IUnknown *LPUNKNOWN;

HRESULT CoCreateInstance(
  REFCLSID  rclsid,
  LPUNKNOWN pUnkOuter,
  DWORD     dwClsContext,
  REFIID    riid,
  LPVOID    *ppv
);
