/*
 * Copyright (C) 2016-2017 Tung Nguyen Thanh.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 This file defines:
 - The entry point of the program: function OnInit.
 - The public methods of class FMIP.
 - And other functions that the methods of class FMIP call to.
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

bool FMIP::OnInit() // Entry point of the program.
{
#ifdef _NDEBUG
	// If there is an instance of the same instance running, bring it to the foreground and then terminate.
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
	if (!LookupPrivilegeValue(
		nullptr, // lookup privilege on local system
		lpszPrivilege, // privilege to lookup 
		&luid)) // receives LUID of privilege
	{
		wxLogDebug(wxString::Format(L"LookupPrivilegeValue error: %d", GetLastError()));
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
		wxLogDebug(wxString::Format(L"AdjustTokenPrivileges error: ", GetLastError()));
		return FALSE;
	}

	if (GetLastError() == ERROR_NOT_ALL_ASSIGNED)
	{
		wxLogDebug(L"The token does not have the specified privilege.");
		return FALSE;
	}
	return TRUE;
}

typedef BOOL(WINAPI *LPFN_ISWOW64PROCESS)(HANDLE, PBOOL);
LPFN_ISWOW64PROCESS fnIsWow64Process;

BOOL FMIP::IsProcessWoW64(const HANDLE hProcess)
{
	BOOL bIsWow64 = FALSE;
	//IsWow64Process is not available on all supported instances of Windows.
	//Use GetModuleHandle to get a handle to the DLL that contains the function
	//and GetProcAddress to get a pointer to the function if available.
	fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(// = (BOOL(WINAPI *) (HANDLE, PBOOL))GetProcAddress
		GetModuleHandle(TEXT("kernel32")), "IsWow64Process");
	if (nullptr != fnIsWow64Process)
		if (!fnIsWow64Process(hProcess, &bIsWow64))
		{
			wxLogDebug(L"Could not find function \"IsWow64Process\" in module \"kernel32.exe\".");
			return FALSE;
		}
	return bIsWow64;
}

/*
 This function scans from the VA given as the second argument 
 until it finds a commited private region with protection attribute of XRW 
 - the indication of being injected code into the process: then it returns the virtual address of the suspicious region;
 or until it finishes the user-mode VM partition: then it returns null address, that means it finds none.
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

			// current protection is XR but initial protection is RW
			// that means after having written code into the process, the injector changes the protection of the region
			// to bypass security tools hunting for private memory regions with XRW protection.
			if (mbi.AllocationProtect == PAGE_READWRITE && mbi.Protect == PAGE_EXECUTE_READ) 
			{
				return mbi.BaseAddress;
			}

			// Initially, the allocated memory is for executing code. But, after finishing execution, (or it is just some information with no need for execution, e.g header),
			// it's protection is changed to bypass security tools hunting for private memory regions with XRW protection.
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
 This function sends requests and relevant data from one instance to the other instance (X86 and X64) of this program.
 It is used by other functions to request a specific action.
 At present, we don't use the result it returns.
*/
BOOL SendRequestToRemoteInstance(HWND hwndDestWindow, HWND hwndSourceWindow, ACTION Action, LPVOID ptrIPCData, DWORD DataSize)
{
	// if there is no destination window, do nothing.
	if (hwndDestWindow == nullptr)
		return FALSE;

	COPYDATASTRUCT CopyDataStruct;
	CopyDataStruct.dwData = Action;
	CopyDataStruct.cbData = DataSize;
	CopyDataStruct.lpData = ptrIPCData;
	return SendMessage(hwndDestWindow, WM_COPYDATA, (WPARAM)hwndSourceWindow, (LPARAM)&CopyDataStruct);
}

/*
This function sends a request for making a tree item from X86 instance to the X64 instance.
*/
BOOL SendTreeItemToX64Instance(HWND hwndDestWindow, TREE_ITEM_PROPERTIES* ptrNodeProperties, DWORD DataSize)
{
	return SendRequestToRemoteInstance(hwndDestWindow, nullptr, MAKE_TREE_ITEM_IN_X64_INSTANCE, ptrNodeProperties, DataSize);
}

/*
This function sends a request from the X64 instance to the X86 instance for handling a X86 Wow process.
*/
BOOL RequestX86Handling(HWND hwndDestWindow, HWND hwndSourceWindow, PROCESS_NAME_AND_PID* ptrProcessNamePId, DWORD DataSize)
{
	return SendRequestToRemoteInstance(hwndDestWindow, hwndSourceWindow, REQUEST_X86HANDLING_TO_X86_INSTANCE, ptrProcessNamePId, DataSize);
}

void MakeTreeItemsAboutAProcess(HWND hwndDestWindow, wxTreeCtrl* ptrTreeCtrl, const wxTreeItemId& tiRoot, HANDLE hProcess, const PROCESS_NAME_AND_PID& ProcessNamePId)
{
	LPCVOID pcvoidRegionBase = (LPCVOID)0x10000; // start scanning from user-mode VM partition. 
	LPCVOID pcvoidAllocationBase = nullptr;
	MEMORY_BASIC_INFORMATION mbi;
	TREE_ITEM_PROPERTIES NodeProperties;
	pcvoidRegionBase = FMIP::FindPrivateERWRegion(hProcess, pcvoidRegionBase);
	if (pcvoidRegionBase != nullptr) // found the first private ERW Region.
	{
		// variable names: prefix ti stands for Tree Item.
		wxTreeItemId tiLastProcessNameId = 0;
		wxTreeItemId tiLastAllocationBase = 0;
		wxTreeItemId tiLastRegion = 0;
		TREE_ITEM_PARENT_DATA_TO_CHANGE tiParentDataToChange;
		BOOL blIsPEInjectedInProcess = FALSE;
		NodeProperties.blPEInjection = FALSE;
		VirtualQueryEx(hProcess, pcvoidRegionBase, &mbi, sizeof(MEMORY_BASIC_INFORMATION));
		pcvoidRegionBase = mbi.AllocationBase;
		if (hwndDestWindow != nullptr) // if there is a remote (X64) instance then tell it to append a tree item to display the process' name and PId,...
		{
			wxString wxszItemText;
			NodeProperties.TREEITEMTYPE = PROCESS_NAME_WITH_PID;
			NodeProperties.PROCESSNAMEPID.dwPId = ProcessNamePId.dwPId;
			wxszItemText = ProcessNamePId.wszProcessName;
			wxszItemText.append(wxString::Format(wxT(" (PId: %d X86)"), ProcessNamePId.dwPId));
			DWORD dwLength = wxszItemText.length();
			for (DWORD i = 0; i <= dwLength; i++) // include NULL character.
			{
				NodeProperties.PROCESSNAMEPID.wszProcessName[i] = wxszItemText.wc_str()[i];
			}
			SendTreeItemToX64Instance(hwndDestWindow, &NodeProperties, sizeof(TREE_ITEM_PROPERTIES));
		}
		else // ...else display the name and Pid of the process.
		{
			wxTreeItemData* ptrTreeItemProcessNamePId = new Tree_Item_Process_Name_PId(ProcessNamePId.dwPId);
			tiLastProcessNameId = ptrTreeCtrl->AppendItem(tiRoot, wxString::Format(wxT("%s (PId: %d)"), ProcessNamePId.wszProcessName, ProcessNamePId.dwPId), -1, -1, ptrTreeItemProcessNamePId);
		}
		do // make tree items of a allocation base and their child regions.
		{
			VirtualQueryEx(hProcess, pcvoidRegionBase, &mbi, sizeof(MEMORY_BASIC_INFORMATION)); // query data for displaying a allocation base tree item.
			pcvoidAllocationBase = mbi.AllocationBase;
			if (hwndDestWindow != nullptr) // if there is a remote (X64) instance then tell it to display the region's allocation base,...
			{
				NodeProperties.TREEITEMTYPE = ALLOCATION_BASE;
				NodeProperties.ptr32AllocationBase = (VOID* POINTER_32)pcvoidAllocationBase;
				SendTreeItemToX64Instance(hwndDestWindow, &NodeProperties, sizeof(TREE_ITEM_PROPERTIES));
			}
			else // ...else, display the region's allocation base.
			{
				Tree_Item_Allocation_Base* ptrTreeItemAllocationBase = new Tree_Item_Allocation_Base(mbi.AllocationBase);
				tiLastAllocationBase = ptrTreeCtrl->AppendItem(tiLastProcessNameId, wxString::Format(wxT("0x%p"), pcvoidAllocationBase), -1, -1, static_cast<wxTreeItemData*>(ptrTreeItemAllocationBase));
				ptrTreeCtrl->SetItemFont(tiLastAllocationBase, wxFont(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT("Consolas")));
			}
			while ((mbi.AllocationBase == pcvoidAllocationBase) /*|| (mbi.AllocationBase == (LPVOID)NodeProperties.AllocationBase)*/) // to find all regions belonging to the same allocation base.
			{
				if (mbi.State == MEM_COMMIT)
				{
					// Check if this process has an indication of PE injection.
					SIZE_T nNumOfBytesRead;
					std::unique_ptr<BYTE>smptrAutoFreedByteBuffer(new BYTE[mbi.RegionSize]);
					PBYTE ptrByteBuffer = smptrAutoFreedByteBuffer.get(); 
					ReadProcessMemory(hProcess, pcvoidRegionBase, (PVOID)ptrByteBuffer, mbi.RegionSize, &nNumOfBytesRead);
					for (SIZE_T i = 0; i < mbi.RegionSize; i++) // assume i is the start of a DOS header.
					{
						// No need for std::regex because we don't have to do much text manipulation. Moreover, we're dealing with bytes, not strings.
						if (ptrByteBuffer[i] == 'M')
						{
							if (i + 1 >= mbi.RegionSize)
								break; // impossible to be a DOS header in this region, stop scanning for PE signature.
							if (ptrByteBuffer[i + 1] == 'Z')
							{
								if (i + 0x3c >= mbi.RegionSize)
									break; // impossible to be a DOS header in this region, stop scanning for PE signature.
								DWORD dwPEOffset = *(PDWORD)&ptrByteBuffer[i + 0x3c];
								if (i + dwPEOffset + 1 >= mbi.RegionSize)
									break; // impossible to be a PE header in this region, stop scanning for PE signature.
								if (ptrByteBuffer[i + dwPEOffset] == 'P' && ptrByteBuffer[i + dwPEOffset + 1] == 'E')
								{
									// Found an indication of PE injection.
									NodeProperties.blPEInjection = TRUE; // this region is is injected with a PE.
									blIsPEInjectedInProcess = TRUE; // the currently examined process is injected with a PE.
									if (hwndDestWindow != nullptr) // if there is a remote (X64) instance then tell it to  change the color of the last tree item displaying the region's allocation base to red,...
									{
										tiParentDataToChange.TreeItemParentType = ALLOCATION_BASE;
										tiParentDataToChange.wxclColor.Set(255, 0, 0);
										SendRequestToRemoteInstance(hwndDestWindow, nullptr, CHANGE_LAST_TREE_ITEM_PARENT_DATA_IN_X64_INSTANCE, &tiParentDataToChange, sizeof(TREE_ITEM_PARENT_DATA_TO_CHANGE));
									}
									else // ...else, change the color of the last tree item displaying the the region's allocation base to red.
									{
										wxTreeItemData* ptiData = ptrTreeCtrl->GetItemData(tiLastAllocationBase);
										static_cast<Tree_Item_Allocation_Base*>(ptiData)->SetColor(255, 0, 0);
									}
									break; // stop scanning for PE signature in this region.
								}
							}
						}
					}
					if (hwndDestWindow != nullptr) // if there is a remote (X64) instance then tell it display the region adress.,...
					{
						NodeProperties.TREEITEMTYPE = REGION;
						NodeProperties.ptr32BaseAddress = (VOID* POINTER_32)mbi.BaseAddress;
						NodeProperties.dwRegionSize = mbi.RegionSize;
						SendTreeItemToX64Instance(hwndDestWindow, &NodeProperties, sizeof(TREE_ITEM_PROPERTIES));
					}
					else // ...or else, display the region adress.
					{
						Tree_Item_Region* ptrTreeItemRegion = new Tree_Item_Region(mbi.BaseAddress, mbi.RegionSize);
						if (NodeProperties.blPEInjection)
							ptrTreeItemRegion->SetColor(255, 0, 0); // make a warning by displaying this region in red.
						tiLastRegion = ptrTreeCtrl->AppendItem(tiLastAllocationBase, wxString::Format(wxT("0x%p"), pcvoidRegionBase), -1, -1, static_cast<wxTreeItemData*>(ptrTreeItemRegion));
						ptrTreeCtrl->SetItemFont(tiLastRegion, wxFont(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT("Consolas")));
					}
				}
				pcvoidRegionBase = (PBYTE)(mbi.BaseAddress) + mbi.RegionSize;
				VirtualQueryEx(hProcess, pcvoidRegionBase, &mbi, sizeof(MEMORY_BASIC_INFORMATION));
			}
			NodeProperties.blPEInjection = FALSE; // assume that the next region is not injected with a PE.
			pcvoidRegionBase = FMIP::FindPrivateERWRegion(hProcess, pcvoidRegionBase);
		} while (pcvoidRegionBase != nullptr); // keep searching until reaching the end of user-mode VM partition.
		
		if (blIsPEInjectedInProcess == TRUE) 
			if (hwndDestWindow != nullptr) // if there is a remote (X64) instance then tell it to change the color of the last tree item displaying the process name and PId to red,...
			{
				tiParentDataToChange.TreeItemParentType = PROCESS_NAME_WITH_PID;
				tiParentDataToChange.wxclColor.Set(255, 0, 0);
				SendRequestToRemoteInstance(hwndDestWindow, nullptr, CHANGE_LAST_TREE_ITEM_PARENT_DATA_IN_X64_INSTANCE, &tiParentDataToChange, sizeof(TREE_ITEM_PARENT_DATA_TO_CHANGE));
			}
			else // ...else, change the local last tree item displaying the process name and PId to red.
			{
				wxTreeItemData* ptiData = ptrTreeCtrl->GetItemData(tiLastProcessNameId);
				static_cast<Tree_Item_Process_Name_PId*>(ptiData)->SetColor(255, 0, 0);
			}
		}
	}

void MakeTreeItemsInLocalInstance(wxTreeCtrl* ptrTreeCtrl, const wxTreeItemId& tiRoot, HANDLE hProcess, const PROCESS_NAME_AND_PID&  ProcessNamePId)
{
	MakeTreeItemsAboutAProcess(nullptr, ptrTreeCtrl, tiRoot, hProcess, ProcessNamePId);
}

void FMIP::MakeTreeItemsInRemoteInstance(HWND hwndDestWindow, HANDLE hProcess, const PROCESS_NAME_AND_PID&  ProcessNamePId)
{
	MakeTreeItemsAboutAProcess(hwndDestWindow, nullptr, nullptr, hProcess, ProcessNamePId);
}

BOOL FMIP::FillTreeCtrl(FMIP_TreeCtrl* ptrTreeCtrl)
{
	HANDLE hSnapshot = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	PROCESSENTRY32 pe32;
	pe32.dwSize = sizeof(PROCESSENTRY32); // needed.
	if (!Process32First(hSnapshot, &pe32))
	{
		OutputDebugString(L"No running processes??\n");
		return false;
	}
	HANDLE hProcess = nullptr;
	wxTreeItemId tiRoot = ptrTreeCtrl->AddRoot(wxT("Root"));

#ifdef _WIN64 // our app is X64, so...
	HWND hwndX86Handler;	// handler in X86 (remote) instance.
	hwndX86Handler = ::FindWindow(nullptr, AppTitleWoW);	// find the X86 instance to send VM queries for X86 processes
	HWND hwndX64MakeTreeItemsHandler = ptrTreeCtrl->GetHWND(); // handler in X64 instance.
#endif
	do
	{
		hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe32.th32ProcessID);
		if ((hProcess == INVALID_HANDLE_VALUE) || (hProcess == nullptr))
		{
			wxLogDebug(wxString::Format(L"Unable to open process %s, please run as Administrator.", pe32.szExeFile));
			continue;
		}
		PROCESS_NAME_AND_PID PROCESSNAMEPID;
		PROCESSNAMEPID.dwPId = pe32.th32ProcessID;
		wxString strProcessName(pe32.szExeFile);
		DWORD dwLength = strProcessName.length();
		for (DWORD i = 0; i <= dwLength; i++) // include NULL character.
		{
			PROCESSNAMEPID.wszProcessName[i] = strProcessName.wc_str()[i];
		}
#ifdef _WIN64 // our app is X64 and...
		if (FMIP::IsProcessWoW64(hProcess) == TRUE) // the process to be examined (with hProcess) is a X86 one running under WoW64, so...
		{
			// send the information about the process to be examined to the handler in X86 instance.
			RequestX86Handling(hwndX86Handler, hwndX64MakeTreeItemsHandler, &PROCESSNAMEPID, sizeof(PROCESS_NAME_AND_PID));
		}
		else // else, just make tree items with information about the process being examine.
		{
			MakeTreeItemsInLocalInstance(ptrTreeCtrl, tiRoot, hProcess, PROCESSNAMEPID);
		}
#else
		MakeTreeItemsInLocalInstance(ptrTreeCtrl, tiRoot, hProcess, PROCESSNAMEPID);
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

void Generic_Tree_Item::SetColor(unsigned char red, unsigned char green, unsigned char blue)
{
	m_wxclColor.Set(red, green, blue);
}

wxColor * Generic_Tree_Item::GetColor()
{
	return &m_wxclColor;
}

TREE_ITEM_TYPE Generic_Tree_Item::GetType() const
{
	return m_TreeItemType;
}

Generic_Tree_Item::~Generic_Tree_Item()
{
}

Tree_Item_Process_Name_PId::Tree_Item_Process_Name_PId(DWORD PId)
{
	SetType(PROCESS_NAME_WITH_PID);
	this->m_dwPId = PId;
}

DWORD Tree_Item_Process_Name_PId::GetPId()
{
	return this->m_dwPId;
}

Tree_Item_Allocation_Base::Tree_Item_Allocation_Base(LPCVOID AllocationBase)
{
	SetType(ALLOCATION_BASE);
	this->m_pcvoidAllocationBase = AllocationBase;
}

LPCVOID Tree_Item_Allocation_Base::GetAllocationBase()
{
	return m_pcvoidAllocationBase;
}

Tree_Item_Region::Tree_Item_Region(LPCVOID BaseAddress, SIZE_T RegionSize)
{
	SetType(REGION);
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