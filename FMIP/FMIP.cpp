#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include "FMIP.h"
#include "MainWindow.h"
#include <TlHelp32.h>
#include <Capstone\headers\capstone.h>
#include <strsafe.h>

#ifdef NDEBUG
#ifdef _WIN64
#pragma comment(lib,"capstoneX64.lib")
#else
#pragma comment(lib,"capstonex86.lib")
#endif // _WIN64
#else
#ifndef _WIN64
#pragma comment(lib,"capstone_dll.lib")
#endif // !_WIN64
#endif // NDEBUG

#ifdef __WXMSW__
#include <wx/msw/msvcrt.h>      // redefines the new() operator 
#endif

bool ThisApp::OnInit()
{
	HWND hWnd = ::FindWindow(nullptr, AppTitle);
	if (hWnd != NULL)
	{
		::SetActiveWindow(hWnd);
		::ShowWindow(hWnd, SW_NORMAL);
		return false;
	}
	HANDLE hToken;
	OpenProcessToken(GetCurrentProcess(), TOKEN_ALL_ACCESS, &hToken);
	FMIP::SetPrivilege(hToken, L"SeDebugPrivilege", TRUE);
	::CloseHandle(hToken);
	MainWindow* ptrWindow = new MainWindow(wxT("Tìm tiến trình nghi vấn bị tiêm mã độc"));
	return true;
}

BOOL FMIP::SetPrivilege(
	HANDLE hToken, LPCTSTR lpszPrivilege, BOOL bEnablePrivilege
)
{
	TOKEN_PRIVILEGES tp;
	LUID luid;
	TCHAR szOutput[512];
	if (!LookupPrivilegeValue(
		nullptr, // lookup privilege on local system
		lpszPrivilege, // privilege to lookup 
		&luid)) // receives LUID of privilege
	{
		StringCchPrintf(szOutput, sizeof(szOutput) / sizeof(TCHAR), L"%s: %u\n", L"LookupPrivilegeValue error:", GetLastError());
		OutputDebugString(szOutput);
		return FALSE;
	}

	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	if (bEnablePrivilege)
		tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	else
		tp.Privileges[0].Attributes = 0;

	// Enable the privilege or disable all privileges.
	if (!AdjustTokenPrivileges(
		hToken,
		FALSE,
		&tp,
		sizeof(TOKEN_PRIVILEGES),
		(PTOKEN_PRIVILEGES)nullptr,
		(PDWORD)nullptr))
	{
		StringCchPrintf(szOutput, sizeof(szOutput) / sizeof(TCHAR), L"%s: %u\n", L"AdjustTokenPrivileges error:", GetLastError());
		OutputDebugString(szOutput);
		return FALSE;
	}

	if (GetLastError() == ERROR_NOT_ALL_ASSIGNED)
	{
		StringCchPrintf(szOutput, sizeof(szOutput) / sizeof(TCHAR), L"The token does not have the specified privilege. \n");
		OutputDebugString(szOutput);
		return FALSE;
	}
	return TRUE;
}

typedef BOOL(WINAPI *LPFN_ISWOW64PROCESS)(HANDLE, PBOOL);
LPFN_ISWOW64PROCESS fnIsWow64Process;

BOOL FMIP::IsProcessWoW64(const HANDLE hProcess)
{
	BOOL bIsWow64 = FALSE;
	//IsWow64Process is not available on all supported versions of Windows.
	//Use GetModuleHandle to get a handle to the DLL that contains the function
	//and GetProcAddress to get a pointer to the function if available.
	fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(// = (BOOL(WINAPI *) (HANDLE, PBOOL))GetProcAddress
		GetModuleHandle(TEXT("kernel32")), "IsWow64Process");
	if (nullptr != fnIsWow64Process)
		if (!fnIsWow64Process(hProcess, &bIsWow64))
		{
			//handle error
		}
	return bIsWow64;
}

LPCVOID FMIP::FindPrivateERWRegion(HANDLE hProcess, LPCVOID ptrRegionBase)
{
	MEMORY_BASIC_INFORMATION mbi;
	while (VirtualQueryEx(hProcess, ptrRegionBase, (PMEMORY_BASIC_INFORMATION)&mbi, sizeof(MEMORY_BASIC_INFORMATION)) != 0)
	{
		if (mbi.Type == MEM_PRIVATE && mbi.State == MEM_COMMIT)
		{
			if (mbi.AllocationProtect == PAGE_EXECUTE_READWRITE)
			{
				return mbi.BaseAddress;
			}
			if (mbi.AllocationProtect == PAGE_READWRITE && mbi.Protect == PAGE_EXECUTE_READ)
			{
				return mbi.BaseAddress;
			}
			if (mbi.AllocationProtect == PAGE_EXECUTE_READ && mbi.Protect == PAGE_READWRITE)
			{
				return mbi.BaseAddress;
			}
		}
		ptrRegionBase = (PBYTE)ptrRegionBase + mbi.RegionSize;
	} // end of while loop
	return nullptr;
}

BOOL RequestAction(HWND hwndDestWindow, HWND hwndSourceWindow, ACTION Action, LPVOID ptrIPCData, DWORD DataSize)
{
	if (hwndDestWindow == nullptr)
		return FALSE;
	COPYDATASTRUCT CopyDataStruct;
	CopyDataStruct.dwData = Action;
	CopyDataStruct.cbData = DataSize;
	CopyDataStruct.lpData = ptrIPCData;
	return SendMessage(hwndDestWindow, WM_COPYDATA, (WPARAM)hwndSourceWindow, (LPARAM)&CopyDataStruct);
}

BOOL SendTreeItem(HWND hwndDestWindow, TREE_ITEM_PROPERTIES* ptrNodeProperties, DWORD DataSize)
{
	return RequestAction(hwndDestWindow, nullptr, ACTION_MAKE_TREE_ITEMS, ptrNodeProperties, DataSize);
}

BOOL RequestX86Handling(HWND hwndDestWindow, HWND hwndSourceWindow, PROCESS_NAME_PID* ptrProcessNamePId, DWORD DataSize)
{
	return RequestAction(hwndDestWindow, hwndSourceWindow, ACTION_REQUEST_FOR_X86HANDLING, ptrProcessNamePId, DataSize);
}

void MakeTreeNodesForProcess(HWND hwndDestWindow, wxTreeCtrl* ptrTreeCtrl, const wxTreeItemId& tiRoot, HANDLE hProcess, const PROCESS_NAME_PID& ProcessNamePId)
{
	LPCVOID ptrRegionBase = (LPCVOID)0x10000; // start scanning from user-mode VM partition 
	LPCVOID ptrAllocationBase = nullptr;
	MEMORY_BASIC_INFORMATION mbi;
	TREE_ITEM_PROPERTIES NodeProperties;
	ptrRegionBase = FMIP::FindPrivateERWRegion(hProcess, ptrRegionBase);
	if (ptrRegionBase != nullptr) // found the first private ERW Region
	{
		// variable names: prefix ti stands for Tree Item
		wxTreeItemId tiLastProcessNameId = 0;
		wxTreeItemId tiLastAllocationBase = 0;
		wxTreeItemId tiLastRegion = 0;
		BOOL blIsPEInjectedInProcess = FALSE;
		NodeProperties.blPEInjection = FALSE;
		TREE_ITEM_TYPE tiType;
		VirtualQueryEx(hProcess, ptrRegionBase, &mbi, sizeof(MEMORY_BASIC_INFORMATION));
		ptrRegionBase = mbi.AllocationBase;
		if (hwndDestWindow != nullptr) // if there is a remote (X86) version then send relevant data to the remote version...
		{
			wxString wxszItemText;
			NodeProperties.TREEITEMTYPE = TREE_ITEM_TYPE_PROCESS_NAME_PID;
			NodeProperties.PROCESSNAMEPID.dwPId = ProcessNamePId.dwPId;
			wxszItemText = ProcessNamePId.szProcessName;
			wxszItemText.append(wxString::Format(wxT(" (PId: %d X86)"), ProcessNamePId.dwPId));
			DWORD dwLength = wxszItemText.length();
			for (DWORD i = 0; i <= dwLength; i++) // include NULL character
			{
				NodeProperties.PROCESSNAMEPID.szProcessName[i] = wxszItemText.wc_str()[i];
			}
			SendTreeItem(hwndDestWindow, &NodeProperties, sizeof(TREE_ITEM_PROPERTIES));
		}
		else // ...else display the name of the process
		{
			wxTreeItemData* ptrTreeItemProcessNamePId = new Tree_Item_ptrrocess_Name_PId(ProcessNamePId.dwPId);
			tiLastProcessNameId = ptrTreeCtrl->AppendItem(tiRoot, wxString::Format(wxT("%s (PId: %d)"), ProcessNamePId.szProcessName, ProcessNamePId.dwPId), -1, -1, ptrTreeItemProcessNamePId);
		}
		do // make tree items of a allocation base and their child regions
		{
			VirtualQueryEx(hProcess, ptrRegionBase, &mbi, sizeof(MEMORY_BASIC_INFORMATION)); // query data for displaying a allocation base tree item
			ptrAllocationBase = mbi.AllocationBase;
			//if (!blIsPEInjectedInProcess)
			////{
			////	// Check if this process has a sign of PE injection
			////	SIZE_T nNumOfBytesRead;
			////	char* ptrCharBuffer = new char[mbi.RegionSize];
			////	ReadProcessMemory(hProcess, (PBYTE)ptrRegionBase, ptrCharBuffer, mbi.RegionSize, &nNumOfBytesRead);
			////	int i;
			////	for (i = 0; i < mbi.RegionSize; i++)
			////	{
			////		if (ptrCharBuffer[i] == 'M')
			////		{
			////			if (i + 1 >= mbi.RegionSize)
			////				break;
			////			if (ptrCharBuffer[i + 1] == 'Z')
			////			{
			////				if (i + 0x3c >= mbi.RegionSize)
			////					break;
			////				DWORD dwPEOffset = *(PDWORD)((PBYTE)ptrCharBuffer + i + 0x3c);
			////				if (i + dwPEOffset + 1 >= mbi.RegionSize)
			////					break;
			////			/*	BYTE b = ptrCharBuffer[i + dwPEOffset];
			////				OutputDebugString(L"");*/
			////				if (ptrCharBuffer[i + dwPEOffset] == 'P' && ptrCharBuffer[i + dwPEOffset + 1] == 'E')
			////				{
			////					NodeProperties.blPEInjection = TRUE;
			////					blIsPEInjectedInProcess = TRUE;
			////					break;
			////				}
			////			}
			////		}
			////	}
			//	/*if (ptrCharBuffer[0] == 'M' && ptrCharBuffer[1] == 'Z')
			//	{
			//		NodeProperties.blPEInjection = TRUE;
			//		blIsPEInjectedInProcess = TRUE;
			//	}*/
			//}
			if (hwndDestWindow != nullptr)
			{
				NodeProperties.TREEITEMTYPE = TREE_ITEM_TYPE_ALLOCATION_BASE;
				NodeProperties.dwAllocationBase = (DWORD)ptrAllocationBase;
				SendTreeItem(hwndDestWindow, &NodeProperties, sizeof(TREE_ITEM_PROPERTIES));
			}
			else
			{
				Tree_Item_Allocation_Base* ptrTreeItemParentRegion = new Tree_Item_Allocation_Base(mbi.AllocationBase);
				ptrTreeItemParentRegion->SetRedWarning(NodeProperties.blPEInjection);
				tiLastAllocationBase = ptrTreeCtrl->AppendItem(tiLastProcessNameId, wxString::Format(wxT("0x%p"), ptrAllocationBase), -1, -1, static_cast<wxTreeItemData*>(ptrTreeItemParentRegion));
				ptrTreeCtrl->SetItemFont(tiLastAllocationBase, wxFont(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT("Consolas")));
				//if (NodeProperties.blPEInjection) ptrTreeCtrl->SetItemTextColour(tiLastAllocationBase, wxColor(255, 0, 0));
			}
			while ((mbi.AllocationBase == ptrAllocationBase) /*|| (mbi.AllocationBase == (LPVOID)NodeProperties.AllocationBase)*/) // to find all regions belonging to the same allocation base (allocation base)
			{
				if (mbi.State == MEM_COMMIT)
				{
					// Check if this process has a sign of PE injection
					SIZE_T nNumOfBytesRead;
					char* ptrCharBuffer = new char[mbi.RegionSize];
					ReadProcessMemory(hProcess, (PBYTE)ptrRegionBase, ptrCharBuffer, mbi.RegionSize, &nNumOfBytesRead);
					int i;
					for (i = 0; i < mbi.RegionSize; i++)
					{
						if (ptrCharBuffer[i] == 'M')
						{
							if (i + 1 >= mbi.RegionSize)
								break;
							if (ptrCharBuffer[i + 1] == 'Z')
							{
								if (i + 0x3c >= mbi.RegionSize)
									break;
								DWORD dwPEOffset = *(PDWORD)((PBYTE)ptrCharBuffer + i + 0x3c);
								if (i + dwPEOffset + 1 >= mbi.RegionSize)
									break;
								/*	BYTE b = ptrCharBuffer[i + dwPEOffset];
									OutputDebugString(L"");*/
								if (ptrCharBuffer[i + dwPEOffset] == 'P' && ptrCharBuffer[i + dwPEOffset + 1] == 'E')
								{
									NodeProperties.blPEInjection = TRUE;
									blIsPEInjectedInProcess = TRUE;
									if (hwndDestWindow != nullptr)
									{
										tiType = TREE_ITEM_TYPE_ALLOCATION_BASE;
										RequestAction(hwndDestWindow, nullptr, ACTION_EDIT_LAST_TREE_ITEM, &tiType, sizeof(TREE_ITEM_TYPE_ALLOCATION_BASE));
									}
									else
									{
										wxTreeItemData* ptiData = ptrTreeCtrl->GetItemData(tiLastAllocationBase);
										static_cast<Tree_Item_Allocation_Base*>(ptiData)->SetRedWarning(NodeProperties.blPEInjection);
									}
									break;
								}
							}
						}
					}
					if (hwndDestWindow != nullptr)
					{
						NodeProperties.TREEITEMTYPE = TREE_ITEM_TYPE_REGION;
						NodeProperties.dwBaseAddress = (DWORD)mbi.BaseAddress;
						NodeProperties.lnRegionSize = mbi.RegionSize;
						SendTreeItem(hwndDestWindow, &NodeProperties, sizeof(TREE_ITEM_PROPERTIES));
					}
					else
					{
						Tree_Item_Region* ptrTreeItemRegion = new Tree_Item_Region(mbi.BaseAddress, mbi.RegionSize);
						ptrTreeItemRegion->SetRedWarning(NodeProperties.blPEInjection);
						tiLastRegion = ptrTreeCtrl->AppendItem(tiLastAllocationBase, wxString::Format(wxT("0x%p"), ptrRegionBase), -1, -1, static_cast<wxTreeItemData*>(ptrTreeItemRegion));
						ptrTreeCtrl->SetItemFont(tiLastRegion, wxFont(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT("Consolas")));
						//if (NodeProperties.blPEInjection) ptrTreeCtrl->SetItemTextColour(tiTreeLastRegion, wxColor(255, 0, 0));
					}
				}
				ptrRegionBase = (PBYTE)(mbi.BaseAddress) + mbi.RegionSize;
				VirtualQueryEx(hProcess, ptrRegionBase, &mbi, sizeof(MEMORY_BASIC_INFORMATION));
			}
			NodeProperties.blPEInjection = FALSE;
			ptrRegionBase = FMIP::FindPrivateERWRegion(hProcess, ptrRegionBase);
		} while (ptrRegionBase != nullptr); // keep searching until reach the end of valid VM address
		if (blIsPEInjectedInProcess == TRUE)
		{
			if (hwndDestWindow != nullptr) // if there is a remote (X86) version then send relevant data to the		remote version...
			{
				tiType = TREE_ITEM_TYPE_PROCESS_NAME_PID;
				RequestAction(hwndDestWindow, nullptr, ACTION_EDIT_LAST_TREE_ITEM, &tiType, sizeof(TREE_ITEM_TYPE_PROCESS_NAME_PID));
			}
			else // ...else display the region address
			{
				wxTreeItemData* ptiData = ptrTreeCtrl->GetItemData(tiLastProcessNameId);
				static_cast<Tree_Item_ptrrocess_Name_PId*>(ptiData)->SetRedWarning(blIsPEInjectedInProcess);
				/*wxString wxszItemText = ptrTreeCtrl->GetItemText(tiLastProcessNameId);
				ptrTreeCtrl->SetItemText(tiLastProcessNameId, wxszItemText.append(PEInjectionWarning));*/
				//ptrTreeCtrl->SetItemTextColour(tiLastProcessNameId, wxColor(255, 0, 0));
			}
		}
	}
}

void MakeTreeNodesForCurrentProcess(wxTreeCtrl* ptrTreeCtrl, const wxTreeItemId& tiRoot, HANDLE hProcess, const PROCESS_NAME_PID&  ProcessNamePId)
{
	MakeTreeNodesForProcess(nullptr, ptrTreeCtrl, tiRoot, hProcess, ProcessNamePId);
}

void FMIP::MakeTreeNodesForRemoteProcess(HWND hwndDestWindow, HANDLE hProcess, const PROCESS_NAME_PID&  ProcessNamePId)
{
	MakeTreeNodesForProcess(hwndDestWindow, nullptr, nullptr, hProcess, ProcessNamePId);
}

BOOL FMIP::FillTreeCtrl(FMIP_TreeCtrl* ptrTreeCtrl)
{
	HANDLE hSnapshot = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	PROCESSENTRY32 pe32;
	pe32.dwSize = sizeof(PROCESSENTRY32); // needed
	if (!Process32First(hSnapshot, &pe32))
	{
		OutputDebugString(L"No running processes??\n");
		return false;
	}
	HANDLE hProcess = nullptr;
	wxTreeItemId tiRoot = ptrTreeCtrl->AddRoot(wxT("Root"));

#ifdef _WIN64 // our app is X64, so...
	HWND hwndX86RequestHandler;	// Handler in X86 (remote) version
	hwndX86RequestHandler = ::FindWindow(nullptr, AppTitleWoW);	// find the X86 version to send VM queries for X86 processes
	HWND hwndX64MakeTreeItemsHandler = ptrTreeCtrl->GetHWND(); // Handler in X64 (this/local) version
#endif
	do
	{
		hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe32.th32ProcessID);
		if ((hProcess == INVALID_HANDLE_VALUE) || (hProcess == nullptr))
		{
			continue;
		}
		PROCESS_NAME_PID PROCESSNAMEPID;
		PROCESSNAMEPID.dwPId = pe32.th32ProcessID;
		wxString strProcessName(pe32.szExeFile);
		DWORD dwLength = strProcessName.length();
		for (DWORD i = 0; i <= dwLength; i++) // include NULL character
		{
			PROCESSNAMEPID.szProcessName[i] = strProcessName.wc_str()[i];
		}
#ifdef _WIN64 // our app is X64 and...
		if (FMIP::IsProcessWoW64(hProcess) == TRUE) // this is a X86 process running under WoW64, so...
		{
			RequestAction(hwndX86RequestHandler, hwndX64MakeTreeItemsHandler, ACTION_REQUEST_FOR_X86HANDLING, &PROCESSNAMEPID, sizeof(PROCESS_NAME_PID));
		}
		else
		{
			MakeTreeNodesForCurrentProcess(ptrTreeCtrl, tiRoot, hProcess, PROCESSNAMEPID);
		}
#else
		MakeTreeNodesForCurrentProcess(ptrTreeCtrl, tiRoot, hProcess, PROCESSNAMEPID);
#endif
		CloseHandle(hProcess);
	} while (Process32Next(hSnapshot, &pe32));
	CloseHandle(hSnapshot);
	return TRUE;
}

void Generic_Tree_Item::SetType(const TREE_ITEM_TYPE& TreeItemType)
{
	this->m_TreeItemType = TreeItemType;
}

void Generic_Tree_Item::SetRedWarning(BOOL blToggle)
{
	m_blRedWarning = blToggle;
}

BOOL Generic_Tree_Item::IsRedWarning()
{
	return m_blRedWarning;
}

TREE_ITEM_TYPE Generic_Tree_Item::GetType() const
{
	return m_TreeItemType;
}

Generic_Tree_Item::~Generic_Tree_Item()
{
}

Tree_Item_ptrrocess_Name_PId::Tree_Item_ptrrocess_Name_PId(DWORD PId)
{
	SetType(TREE_ITEM_TYPE_PROCESS_NAME_PID);
	this->m_dwPId = PId;
}

DWORD Tree_Item_ptrrocess_Name_PId::GetPId()
{
	return this->m_dwPId;
}

Tree_Item_Allocation_Base::Tree_Item_Allocation_Base(PVOID AllocationBase)
{
	SetType(TREE_ITEM_TYPE_ALLOCATION_BASE);
	this->m_ptrAllocationBase = AllocationBase;
}

PVOID Tree_Item_Allocation_Base::GetAllocationBase()
{
	return PVOID(m_ptrAllocationBase);
}

Tree_Item_Region::Tree_Item_Region(PVOID BaseAddress, SIZE_T RegionSize)
{
	SetType(TREE_ITEM_TYPE_REGION);
	this->m_ptrBaseAddress = BaseAddress;
	this->m_RegionSize = RegionSize;
}

PVOID Tree_Item_Region::GetBaseAdress()
{
	return PVOID(m_ptrBaseAddress);
}

SIZE_T Tree_Item_Region::GetRegionSize()
{
	return SIZE_T(m_RegionSize);
}

ThisApp::~ThisApp()
{
}