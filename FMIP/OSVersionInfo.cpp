#include "wx/wxprec.h"
#include "OSVersionInfo.h"


OSVersionInfo::OSVersionInfo()
{
}

DWORD OSVersionInfo::GetOSVersion()
{
	OSVERSIONINFOEX OSVerInfo;
	DWORD dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	HMODULE hMod = LoadLibrary(L"ntdll.dll");
	FARPROC pfnRtlGetVersion = nullptr;
	if (hMod != nullptr)
	{
		pfnRtlGetVersion = GetProcAddress(hMod, "RtlGetVersion");
	}
	if (pfnRtlGetVersion != nullptr)
	{
	((void (WINAPI*)(POSVERSIONINFOEX))pfnRtlGetVersion)(&OSVerInfo);
	if (OSVerInfo.dwMajorVersion == 10 || OSVerInfo.wProductType == VER_NT_WORKSTATION)
	{
		return osver
	}
	return 0;
}


OSVersionInfo::~OSVersionInfo()
{
}

