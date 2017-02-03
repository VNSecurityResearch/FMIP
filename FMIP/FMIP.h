/*
/ Opensource project by Tung Nguyen Thanh
/ 2007
*/

#pragma once
#include "FMIP-TreeCtrl.h"
#include <Capstone\headers\capstone.h>

#define PEInjectionWarning wxT(" có PE tiêm trong tiến trình!")
#define AppTitle L"Tìm tiến trình nghi vấn bị tiêm mã độc"
#define AppTitleWoW AppTitle//##" X86"
#define CM_TERMINATE WM_APP

enum TREE_ITEM_TYPE
{
	TREE_ITEM_TYPE_PROCESS_NAME_PID,
	TREE_ITEM_TYPE_ALLOCATION_BASE,
	TREE_ITEM_TYPE_REGION
};

enum ACTION
{
	ACTION_REQUEST_FOR_X86HANDLING,
	ACTION_MAKE_TREE_ITEMS,
	ACTION_CHANGE_TREE_ITEM_PARENT_COLOR
};

struct TREE_ITEM_PARENT_INFO_TO_CHANGE
{
	TREE_ITEM_TYPE TreeItemParentType;
	wxColor wxclColor;
};

struct PROCESS_NAME_PID
{
	std::wstring strProcessName;
	//WCHAR szProcessName[256];
	DWORD dwPId;
};

class Generic_Tree_Item :public wxTreeItemData
{
private:
	TREE_ITEM_TYPE m_TreeItemType;
	wxColor m_wxclColor = wxNullColour;
public:
	//Generic_Tree_Item();
	void SetType(const TREE_ITEM_TYPE& TreeItemType);
	void SetColor(unsigned char, unsigned char, unsigned char);
	wxColor* GetColor();
	TREE_ITEM_TYPE GetType() const;
	virtual ~Generic_Tree_Item() = 0;

};

class Tree_Item_ptrrocess_Name_PId :public Generic_Tree_Item
{
private:
	//PROCESS_NAME_PID ProcessNamePId;
	DWORD m_dwPId;
public:
	Tree_Item_ptrrocess_Name_PId(DWORD PId);
	//~Tree_Item_ptrrocess_Name_PId();
	DWORD GetPId();
};

class Tree_Item_Allocation_Base :public Generic_Tree_Item
{
private:
	PVOID m_ptrAllocationBase;
public:
	Tree_Item_Allocation_Base(PVOID AllocationBase);
	PVOID GetAllocationBase();
};

class Tree_Item_Region :public Generic_Tree_Item
{
private:
	/*PVOID AllocationBase;*/
	PVOID m_ptrBaseAddress;
	SIZE_T m_RegionSize;
	/*DWORD AllocationProtect;
	DWORD State;
	DWORD Protect;*/
public:
	Tree_Item_Region(PVOID BaseAddress, SIZE_T RegionSize);
	PVOID GetBaseAdress();
	SIZE_T GetRegionSize();
};

struct TREE_ITEM_PROPERTIES
{
	PROCESS_NAME_PID PROCESSNAMEPID;
	TREE_ITEM_TYPE TREEITEMTYPE;
	DWORD dwAllocationBase;
	DWORD dwBaseAddress;
	LONG lnRegionSize;
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
	static void MakeTreeNodesForRemoteProcess(HWND hwndDestWindow, HANDLE, const PROCESS_NAME_PID&);
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