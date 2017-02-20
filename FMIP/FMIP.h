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
 This file:
 - defines C NULL terminated strings, constants.
 - defines structures and enumeration types used in the program. 
 - declares classes related to tree items.
 - declares lass FMIP, the primary class of this program.
 - defines a printable character table that the string searching code refers to. 
*/

#pragma once
#include "FMIP-TreeCtrl.h"
#include <Capstone\headers\capstone.h>

//const LPCTSTR PEInjectionWarning = wxT(" có PE tiêm trong tiến trình!");
#define AppTitle		L"Tìm tiến trình nghi vấn bị tiêm mã độc"
#define AppTitleWoW		AppTitle##" X86"
#define CM_TERMINATE	WM_APP
const int MaxFileName = 256 * 2; // length of WCHAR string in byte

enum TREE_ITEM_TYPE
{
	PROCESS_NAME_WITH_PID,
	ALLOCATION_BASE,
	REGION
};

enum ACTION
{
	REQUEST_X86HANDLING_TO_X86_INSTANCE,
	MAKE_TREE_ITEM_IN_X64_INSTANCE,
	CHANGE_LAST_TREE_ITEM_PARENT_DATA_IN_X64_INSTANCE
};

struct TREE_ITEM_PARENT_DATA_TO_CHANGE
{
	TREE_ITEM_TYPE TreeItemParentType;
	wxColor wxclColor;
};

struct PROCESS_NAME_AND_PID
{
	WCHAR wszProcessName[MaxFileName]; 
	DWORD dwPId;
};

class Generic_Tree_Item :public wxTreeItemData
{
private:
	TREE_ITEM_TYPE m_TreeItemType;
	// the customdraw handler will paint this tree item with default color. If it is set to another specific color, then the customdraw handler will paint the tree item accordingly.
	wxColor m_wxclColor = wxNullColour; 
	//
public:
	//Generic_Tree_Item();
	void SetType(const TREE_ITEM_TYPE& TreeItemType);
	void SetColor(unsigned char, unsigned char, unsigned char);
	wxColor* GetColor();
	TREE_ITEM_TYPE GetType() const;
	virtual ~Generic_Tree_Item() = 0;

};

class Tree_Item_Process_Name_PId :public Generic_Tree_Item
{
private:
	//PROCESS_NAME_PID ProcessNamePId;
	DWORD m_dwPId;
public:
	Tree_Item_Process_Name_PId(DWORD PId);
	DWORD GetPId();
};

class Tree_Item_Allocation_Base :public Generic_Tree_Item
{
private:
	LPCVOID m_pcvoidAllocationBase;
public:
	Tree_Item_Allocation_Base(LPCVOID AllocationBase);
	LPCVOID GetAllocationBase();
};

class Tree_Item_Region :public Generic_Tree_Item
{
private:
	/*PVOID AllocationBase;*/
	LPCVOID m_pcvoidBaseAddress;
	SIZE_T m_siztRegionSize;
	/*DWORD AllocationProtect;
	DWORD State;
	DWORD Protect;*/
public:
	Tree_Item_Region(LPCVOID BaseAddress, SIZE_T RegionSize);
	LPCVOID GetBaseAdress();
	SIZE_T GetRegionSize();
};

struct TREE_ITEM_PROPERTIES
{
	PROCESS_NAME_AND_PID PROCESSNAMEPID;
	TREE_ITEM_TYPE TREEITEMTYPE;
	VOID* POINTER_32 ptr32AllocationBase; // notice: because this structure must be...
	VOID* POINTER_32 ptr32BaseAddress;	  // ... exactly of the same size in both the X86 instance and the X64 intance...
	DWORD dwRegionSize;					  // ... so its field sizes must be fixed to guarantee consistency.
	BOOL blPEInjection = FALSE;
};

//namespace FMIP
//{
//	BOOL IsProcessWoW64(HANDLE hProcess);
//	BOOL SetPrivilege(HANDLE hToken, LPCTSTR lpszPrivilege, BOOL bEnablePrivilege);
//	LPCVOID FindPrivateERWRegion(HANDLE hProcess, LPCVOID ptrRegionBase);
//	BOOL FillTreeCtrl(FMIP_TreeCtrl* ptrTreeCtrl);
//	void MakeTreeNodesForRemoteProcess(HWND hwndDestWindow, HANDLE, const PROCESS_NAME_PID&);
//}

class FMIP : public wxApp
{
public:
	virtual bool OnInit() wxOVERRIDE;
	static BOOL IsProcessWoW64(HANDLE hProcess);
	static BOOL SetPrivilege(HANDLE hToken, LPCTSTR lpszPrivilege, BOOL bEnablePrivilege);
	static LPCVOID FindPrivateERWRegion(HANDLE hProcess, LPCVOID ptrRegionBase);
	static BOOL FillTreeCtrl(FMIP_TreeCtrl* ptrTreeCtrl);
	static void MakeTreeItemsInRemoteInstance(HWND hwndDestWindow, HANDLE, const PROCESS_NAME_AND_PID&);
	~FMIP();
};

__declspec(selectany)
BOOLEAN PhCharIsPrintable[256] =
{
	0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0, /* 0 - 15 */ // TAB, LF and CR are printable
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 16 - 31 */
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* ' ' - '/' */
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* '0' - '9' */
	1, 1, 1, 1, 1, 1, 1, /* ':' - '@' */
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 'A' - 'Z' */
	1, 1, 1, 1, 1, 1, /* '[' - '`' */
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 'a' - 'z' */
	1, 1, 1, 1, 0, /* '{' - 127 */ // DEL is not printable
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 128 - 143 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 144 - 159 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 160 - 175 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 176 - 191 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 192 - 207 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 208 - 223 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 224 - 239 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 /* 240 - 255 */
};