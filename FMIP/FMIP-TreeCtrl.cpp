/*
/ Opensource project by Tung Nguyen Thanh
/ 2007
*/

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include "MainWindow.h"
#include "FMIP.h"
#include "VMContentDisplay.h"

#ifdef __WXMSW__
#include <wx/msw/msvcrt.h>      // redefines the new() operator 
#endif

enum POPUP_MENU_ITEM
{
	POPUP_MENU_ITEM_STRING,
	POPUP_MENU_ITEM_ASM,
};

FMIP_TreeCtrl::FMIP_TreeCtrl(wxWindow* parent, wxWindowID Id, long style) : wxTreeCtrl(parent, Id, wxDefaultPosition, wxDefaultSize, style) 
{
}

void FMIP_TreeCtrl::OnPopupClick(wxCommandEvent& event)
{
	
	OUTPUT_TYPE	OutputType = event.GetId() == POPUP_MENU_ITEM_ASM ? OUTPUT_TYPE_ASM : OUTPUT_TYPE_STRING;
	VMContentDisplay* pVMContentDisplay = new VMContentDisplay(static_cast<MainWindow*>(this->GetParent()), this, OutputType);
}

void FMIP_TreeCtrl::OnRightClick(wxTreeEvent& event)
{
	wxTreeItemId TreeItemId = event.GetItem();
	SelectItem(TreeItemId);
	Generic_Tree_Item* ptrGenericTreeItem = static_cast<Generic_Tree_Item*>(this->GetItemData(TreeItemId));
	if (ptrGenericTreeItem->GetType() == TREE_ITEM_TYPE_PROCESS_NAME_PID)
	{
		wxLogDebug(wxT("No context menu on TREE_ITEM_TYPE_PROCESS_NAME_PID"));
		return;
	}
	// Popup menu
	wxMenu pPopupMenu;// = new wxMenu;
	pPopupMenu.Append(POPUP_MENU_ITEM_STRING, wxT("Các dãy ký tự (strings)"));
	pPopupMenu.Append(POPUP_MENU_ITEM_ASM, wxT("Mã hợp ngữ (assembly)"));
	pPopupMenu.Bind(wxEVT_COMMAND_MENU_SELECTED, &FMIP_TreeCtrl::OnPopupClick, this);
	PopupMenu(&pPopupMenu);
}

void FMIP_TreeCtrl::OnRefresh(wxMenuEvent& event)
{
	DeleteAllItems();
	static_cast<MainWindow*>(this->GetParent())->SetStatusText(wxT("Đang quét các tiến trình..."));
	FMIP::FillTreeCtrl(this);
	static_cast<MainWindow*>(this->GetParent())->SetStatusText(wxT("Nhấn chuột phải lên các vùng VM để chọn phương thức hiển thị"));
}

//void FMIP_TreeCtrl::OnKeyDown(wxTreeEvent& event)
//{
//	if (event.GetKeyCode() == 0xd) // Key "Enter"
//	{
//		wxTreeItemId TreeItemId = GetFocusedItem();
//		wxString wxszText = GetItemText(TreeItemId);
//		OutputDebugString(wxszText.wc_str());
//	}
//}

WXLRESULT FMIP_TreeCtrl::MSWWindowProc(WXUINT message, WXWPARAM wParam, WXLPARAM lParam)
{
	// variable names: prefix ti stands for Tree Item
	static wxTreeItemId tiLastProcessNameId = nullptr;
	static wxTreeItemId tiLastAllocationBase = nullptr;
	static wxTreeItemId tiTreeLastRegion = nullptr;
	if (message == WM_COPYDATA)
	{
		COPYDATASTRUCT* ptrCopyDataStruct = (COPYDATASTRUCT*)lParam;
		TREE_ITEM_PROPERTIES* ptrTreeItemProperties = (TREE_ITEM_PROPERTIES*)ptrCopyDataStruct->lpData;
		switch (ptrCopyDataStruct->dwData)
		{
		case ACTION::ACTION_MAKE_TREE_ITEMS:
		{
			wxString wxszItemText;
			switch (ptrTreeItemProperties->TREEITEMTYPE)
			{
			case TREE_ITEM_TYPE::TREE_ITEM_TYPE_PROCESS_NAME_PID:
			{
				wxTreeItemId tiRoot = this->GetRootItem();
				if (tiRoot == nullptr)
				{
					tiRoot = this->AddRoot("Root");
				}
				wxTreeItemData* ptrTreeItemProcessNamePId = new Tree_Item_Process_Name_PId(ptrTreeItemProperties->PROCESSNAMEPID.dwPId);
				tiLastProcessNameId = this->AppendItem(tiRoot, wxString(ptrTreeItemProperties->PROCESSNAMEPID.wszProcessName), -1, -1, ptrTreeItemProcessNamePId);
				break;
			}

			case TREE_ITEM_TYPE::TREE_ITEM_TYPE_ALLOCATION_BASE:
			{
				LPCVOID ptrAllocBaseTruncatedTo32 = (LPCVOID)(UINT_PTR)PtrToUlong(ptrTreeItemProperties->ptr32AllocationBase);
				wxTreeItemData* ptrTreeItemParentRegion = new Tree_Item_Allocation_Base(ptrAllocBaseTruncatedTo32);
				tiLastAllocationBase = this->AppendItem(tiLastProcessNameId, wxString::Format(wxT("0x%p"), ptrAllocBaseTruncatedTo32), -1, -1, ptrTreeItemParentRegion);
				break;
			}

			case TREE_ITEM_TYPE::TREE_ITEM_TYPE_REGION:
			{
				LPCVOID ptrBaseAdrTruncatedTo32 = (LPCVOID)(UINT_PTR)(PtrToUlong(ptrTreeItemProperties->ptr32BaseAddress));
				wxTreeItemData* ptrTreeItemRegion = new Tree_Item_Region(ptrBaseAdrTruncatedTo32, (SIZE_T)ptrTreeItemProperties->siztRegionSize);
				tiTreeLastRegion = this->AppendItem(tiLastAllocationBase, wxString::Format(wxT("0x%p"), ptrBaseAdrTruncatedTo32), -1, -1, ptrTreeItemRegion);
				if (ptrTreeItemProperties->blPEInjection)
					static_cast<Tree_Item_Region*>(ptrTreeItemRegion)->SetColor(255, 0, 0);
				break;
			}

			default:
				break;
			} // close switch (pIPCData->NodeType)
			break;
		}

		case ACTION::ACTION_CHANGE_TREE_ITEM_PARENT_COLOR:
		{
			TREE_ITEM_PARENT_INFO_TO_CHANGE* ptiParentInfoToChange = (TREE_ITEM_PARENT_INFO_TO_CHANGE*)ptrCopyDataStruct->lpData;
			wxTreeItemData* ptiData = nullptr;
			switch (ptiParentInfoToChange->TreeItemParentType)
			{
			case TREE_ITEM_TYPE_PROCESS_NAME_PID:
				ptiData = this->GetItemData(tiLastProcessNameId);
				break;

			case TREE_ITEM_TYPE_ALLOCATION_BASE:
				ptiData = this->GetItemData(tiLastAllocationBase);
				break;

			default:
				break;
			}
			static_cast<Generic_Tree_Item*>(ptiData)->SetColor(ptiParentInfoToChange->wxclColor.Red(), ptiParentInfoToChange->wxclColor.Blue(), ptiParentInfoToChange->wxclColor.Green());
			break;
		}

		default:
			break;
		} // close switch (pCopyDataStruct->dwData)
		return TRUE;
	}
	return wxTreeCtrl::MSWWindowProc(message, wParam, lParam);
}