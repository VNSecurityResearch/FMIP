/*
/ Opensource project by Tung Nguyen Thanh
/ 2007
*/

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include "FMIP.h"
#include "MainWindow.h"
#include <TlHelp32.h>
#include <Capstone\headers\capstone.h>
#include <strsafe.h>
#include <memory>

#ifdef NDEBUG
#ifdef _WIN64
#pragma comment(lib,"capstoneX64.lib")
#else
#pragma comment(lib,"capstonex86.lib")
#endif // _WIN64
#else
#ifndef _WIN64
#pragma comment(lib,"capstoneX86_dll.lib")
#else
#pragma comment(lib,"capstoneX64_dll.lib")
#endif // !_WIN64
#endif // NDEBUG

#ifdef __WXMSW__
#include <wx/msw/msvcrt.h>      // redefines the new() operator 
#endif

bool FMIP::OnInit()
{
#ifdef _NDEBUG
	// If there is an instance of the same version running, bring it to the foreground and terminate.
	HWND hWnd = ::FindWindow(nullptr, AppTitle);
	if (hWnd != NULL)
	{
		::SetForegroundWindow(hWnd);
		return false;
	}
#endif // _NDEBUG

	// Set debug privilege so that we can examine other processes' memory
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

/*
 This function finds a commited private region with protection attribute of XRW - the indication of being injected code into the process.
 Returns the virtual address of the suspicious region or null address if it finds none.
*/
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

/*
 This function sends requests and relevant data from one version to the other version (X86 and X64) of this program.
 It is used by other functions to request specific actions.
 At present, we don't use the result it returns.
*/
BOOL RequestAction(HWND hwndDestWindow, HWND hwndSourceWindow, ACTION Action, LPVOID ptrIPCData, DWORD DataSize)
{
	// if there is no destination window, do nothing
	if (hwndDestWindow == nullptr)
		return FALSE;

	COPYDATASTRUCT CopyDataStruct;
	CopyDataStruct.dwData = Action;
	CopyDataStruct.cbData = DataSize;
	CopyDataStruct.lpData = ptrIPCData;
	return SendMessage(hwndDestWindow, WM_COPYDATA, (WPARAM)hwndSourceWindow, (LPARAM)&CopyDataStruct);
}

/*
This function sends a request for making a tree item from X86 version to the X64 version.
*/
BOOL SendTreeItem(HWND hwndDestWindow, TREE_ITEM_PROPERTIES* ptrNodeProperties, DWORD DataSize)
{
	return RequestAction(hwndDestWindow, nullptr, ACTION_MAKE_TREE_ITEMS, ptrNodeProperties, DataSize);
}

/*
This function sends a request for handling X86 version
*/
BOOL RequestX86Handling(HWND hwndDestWindow, HWND hwndSourceWindow, PROCESS_NAME_PID* ptrProcessNamePId, DWORD DataSize)
{
	return RequestAction(hwndDestWindow, hwndSourceWindow, ACTION_REQUEST_FOR_X86HANDLING, ptrProcessNamePId, DataSize);
}

void MakeTreeNodesForProcess(HWND hwndDestWindow, wxTreeCtrl* ptrTreeCtrl, const wxTreeItemId& tiRoot, HANDLE hProcess, const PROCESS_NAME_PID& ProcessNamePId)
{
	LPCVOID pcvoidRegionBase = (LPCVOID)0x10000; // start scanning from user-mode VM partition 
	LPCVOID pcvoidAllocationBase = nullptr;
	MEMORY_BASIC_INFORMATION mbi;
	TREE_ITEM_PROPERTIES NodeProperties;
	pcvoidRegionBase = FMIP::FindPrivateERWRegion(hProcess, pcvoidRegionBase);
	if (pcvoidRegionBase != nullptr) // found the first private ERW Region
	{
		// variable names: prefix ti stands for Tree Item
		wxTreeItemId tiLastProcessNameId = 0;
		wxTreeItemId tiLastAllocationBase = 0;
		wxTreeItemId tiLastRegion = 0;
		TREE_ITEM_PARENT_INFO_TO_CHANGE tiParentInfoToChange;
		BOOL blIsPEInjectedInProcess = FALSE;
		NodeProperties.blPEInjection = FALSE;
		VirtualQueryEx(hProcess, pcvoidRegionBase, &mbi, sizeof(MEMORY_BASIC_INFORMATION));
		pcvoidRegionBase = mbi.AllocationBase;
		if (hwndDestWindow != nullptr) // this is X86 version sending relevant data to the remote X64 version
		{
			wxString wxszItemText;
			NodeProperties.TREEITEMTYPE = TREE_ITEM_TYPE_PROCESS_NAME_PID;
			NodeProperties.PROCESSNAMEPID.dwPId = ProcessNamePId.dwPId;
			//NodeProperties.PROCESSNAMEPID.strProcessName.append(StringCchPrintfW(L" (PId: %d X86)", ProcessNamePId.dwPId));
			wxszItemText = ProcessNamePId.wszProcessName;
			wxszItemText.append(wxString::Format(wxT(" (PId: %d X86)"), ProcessNamePId.dwPId));
			//NodeProperties.PROCESSNAMEPID.strProcessName = wxszItemText.wc_str();
			DWORD dwLength = wxszItemText.length();
			for (DWORD i = 0; i <= dwLength; i++) // include NULL character
			{
				NodeProperties.PROCESSNAMEPID.wszProcessName[i] = wxszItemText.wc_str()[i];
			}
			SendTreeItem(hwndDestWindow, &NodeProperties, sizeof(TREE_ITEM_PROPERTIES));
		}
		else // ...else display the name of the process
		{
			wxTreeItemData* ptrTreeItemProcessNamePId = new Tree_Item_Process_Name_PId(ProcessNamePId.dwPId);
			tiLastProcessNameId = ptrTreeCtrl->AppendItem(tiRoot, wxString::Format(wxT("%s (PId: %d)"), ProcessNamePId.wszProcessName, ProcessNamePId.dwPId), -1, -1, ptrTreeItemProcessNamePId);
		}
		do // make tree items of a allocation base and their child regions
		{
			VirtualQueryEx(hProcess, pcvoidRegionBase, &mbi, sizeof(MEMORY_BASIC_INFORMATION)); // query data for displaying a allocation base tree item
			pcvoidAllocationBase = mbi.AllocationBase;
			if (hwndDestWindow != nullptr)
			{
				NodeProperties.TREEITEMTYPE = TREE_ITEM_TYPE_ALLOCATION_BASE;
				NodeProperties.ptr32AllocationBase = (VOID* POINTER_32)pcvoidAllocationBase;
				SendTreeItem(hwndDestWindow, &NodeProperties, sizeof(TREE_ITEM_PROPERTIES));
			}
			else
			{
				Tree_Item_Allocation_Base* ptrTreeItemAllocationBase = new Tree_Item_Allocation_Base(mbi.AllocationBase);
				if (NodeProperties.blPEInjection)
					ptrTreeItemAllocationBase->SetColor(255, 0, 0);
				tiLastAllocationBase = ptrTreeCtrl->AppendItem(tiLastProcessNameId, wxString::Format(wxT("0x%p"), pcvoidAllocationBase), -1, -1, static_cast<wxTreeItemData*>(ptrTreeItemAllocationBase));
				ptrTreeCtrl->SetItemFont(tiLastAllocationBase, wxFont(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT("Consolas")));
				//if (NodeProperties.blPEInjection) ptrTreeCtrl->SetItemTextColour(tiLastAllocationBase, wxColor(255, 0, 0));
			}
			while ((mbi.AllocationBase == pcvoidAllocationBase) /*|| (mbi.AllocationBase == (LPVOID)NodeProperties.AllocationBase)*/) // to find all regions belonging to the same allocation base (allocation base)
			{
				if (mbi.State == MEM_COMMIT)
				{
					// Check if this process has a sign of PE injection
					SIZE_T nNumOfBytesRead;
					std::unique_ptr<BYTE>smptrAutoFreedByteBuffer(new BYTE[mbi.RegionSize]);
					PBYTE ptrByteBuffer = smptrAutoFreedByteBuffer.get();
					ReadProcessMemory(hProcess, pcvoidRegionBase, (PVOID)ptrByteBuffer, mbi.RegionSize, &nNumOfBytesRead);
					for ( SIZE_T i = 0; i < mbi.RegionSize; i++)
					{
						if (ptrByteBuffer[i] == 'M')
						{
							if (i + 1 >= mbi.RegionSize)
								break;
							if (ptrByteBuffer[i + 1] == 'Z')
							{
								if (i + 0x3c >= mbi.RegionSize)
									break;
								DWORD dwPEOffset = *(PDWORD)&ptrByteBuffer[i + 0x3c];
								if (i + dwPEOffset + 1 >= mbi.RegionSize)
									break;
								if (ptrByteBuffer[i + dwPEOffset] == 'P' && ptrByteBuffer[i + dwPEOffset + 1] == 'E')
								{
									NodeProperties.blPEInjection = TRUE;
									blIsPEInjectedInProcess = TRUE;
									if (hwndDestWindow != nullptr)
									{
										tiParentInfoToChange.TreeItemParentType = TREE_ITEM_TYPE_ALLOCATION_BASE;
										tiParentInfoToChange.wxclColor.Set(255, 0, 0);
										RequestAction(hwndDestWindow, nullptr, ACTION_CHANGE_TREE_ITEM_PARENT_COLOR, &tiParentInfoToChange, sizeof(TREE_ITEM_PARENT_INFO_TO_CHANGE));
									}
									else
									{
										wxTreeItemData* ptiData = ptrTreeCtrl->GetItemData(tiLastAllocationBase);
										static_cast<Tree_Item_Allocation_Base*>(ptiData)->SetColor(255, 0, 0);
									}
									break;
								}
							}
						}
					}
					if (hwndDestWindow != nullptr)
					{
						NodeProperties.TREEITEMTYPE = TREE_ITEM_TYPE_REGION;
						NodeProperties.ptr32BaseAddress = (VOID* POINTER_32)mbi.BaseAddress;
						NodeProperties.siztRegionSize = mbi.RegionSize;
						SendTreeItem(hwndDestWindow, &NodeProperties, sizeof(TREE_ITEM_PROPERTIES));
					}
					else
					{
						Tree_Item_Region* ptrTreeItemRegion = new Tree_Item_Region(mbi.BaseAddress, mbi.RegionSize);
						if (NodeProperties.blPEInjection)
							ptrTreeItemRegion->SetColor(255, 0, 0);
						tiLastRegion = ptrTreeCtrl->AppendItem(tiLastAllocationBase, wxString::Format(wxT("0x%p"), pcvoidRegionBase), -1, -1, static_cast<wxTreeItemData*>(ptrTreeItemRegion));
						ptrTreeCtrl->SetItemFont(tiLastRegion, wxFont(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT("Consolas")));
						//if (NodeProperties.blPEInjection) ptrTreeCtrl->SetItemTextColour(tiTreeLastRegion, wxColor(255, 0, 0));
					}
				}
				pcvoidRegionBase = (PBYTE)(mbi.BaseAddress) + mbi.RegionSize;
				VirtualQueryEx(hProcess, pcvoidRegionBase, &mbi, sizeof(MEMORY_BASIC_INFORMATION));
			}
			NodeProperties.blPEInjection = FALSE;
			pcvoidRegionBase = FMIP::FindPrivateERWRegion(hProcess, pcvoidRegionBase);
		} while (pcvoidRegionBase != nullptr); // keep searching until reach the end of valid VM address
		if (blIsPEInjectedInProcess == TRUE)
		{
			if (hwndDestWindow != nullptr) // if there is a remote (X86) version then send relevant data to the remote version...
			{
				tiParentInfoToChange.TreeItemParentType = TREE_ITEM_TYPE_PROCESS_NAME_PID;
				tiParentInfoToChange.wxclColor.Set(255, 0, 0);
				RequestAction(hwndDestWindow, nullptr, ACTION_CHANGE_TREE_ITEM_PARENT_COLOR, &tiParentInfoToChange, sizeof(TREE_ITEM_PARENT_INFO_TO_CHANGE));
			}
			else // ...else display the region address
			{
				wxTreeItemData* ptiData = ptrTreeCtrl->GetItemData(tiLastProcessNameId);
				static_cast<Tree_Item_Process_Name_PId*>(ptiData)->SetColor(255, 0, 0);
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
		//PROCESSNAMEPID.strProcessName = pe32.szExeFile;
		wxString strProcessName(pe32.szExeFile);
		DWORD dwLength = strProcessName.length();
		for (DWORD i = 0; i <= dwLength; i++) // include NULL character
		{
			PROCESSNAMEPID.wszProcessName[i] = strProcessName.wc_str()[i];
		}
#ifdef _WIN64 // our app is X64 and...
		if (FMIP::IsProcessWoW64(hProcess) == TRUE) // the process being examined (with hProcess) is a X86 one running under WoW64, so...
		{
			RequestX86Handling(hwndX86RequestHandler, hwndX64MakeTreeItemsHandler, &PROCESSNAMEPID, sizeof(PROCESS_NAME_PID));
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

//void Generic_Tree_Item::SetRedWarning(BOOL blToggle)
//{
//	m_blRedWarning = blToggle;
//}

void Generic_Tree_Item::SetColor(unsigned char red, unsigned char green, unsigned char blue)
{
	m_wxclColor.Set(red, green, blue);
}

wxColor * Generic_Tree_Item::GetColor()
{
	return &m_wxclColor;
}

//BOOL Generic_Tree_Item::IsRedWarning()
//{
//	return m_blRedWarning;
//}

TREE_ITEM_TYPE Generic_Tree_Item::GetType() const
{
	return m_TreeItemType;
}

Generic_Tree_Item::~Generic_Tree_Item()
{
}

Tree_Item_Process_Name_PId::Tree_Item_Process_Name_PId(DWORD PId)
{
	SetType(TREE_ITEM_TYPE_PROCESS_NAME_PID);
	this->m_dwPId = PId;
}

DWORD Tree_Item_Process_Name_PId::GetPId()
{
	return this->m_dwPId;
}

Tree_Item_Allocation_Base::Tree_Item_Allocation_Base(LPCVOID AllocationBase)
{
	SetType(TREE_ITEM_TYPE_ALLOCATION_BASE);
	this->m_pcvoidAllocationBase = AllocationBase;
}

LPCVOID Tree_Item_Allocation_Base::GetAllocationBase()
{
	return m_pcvoidAllocationBase;
}

Tree_Item_Region::Tree_Item_Region(LPCVOID BaseAddress, SIZE_T RegionSize)
{
	SetType(TREE_ITEM_TYPE_REGION);
	this->m_pcvoidBaseAddress = BaseAddress;
	this->m_siztRegionSize = RegionSize;
}

LPCVOID Tree_Item_Region::GetBaseAdress()
{
	return LPCVOID(m_pcvoidBaseAddress);
}

SIZE_T Tree_Item_Region::GetRegionSize()
{
	return SIZE_T(m_siztRegionSize);
}

FMIP::~FMIP()
{
}