/*
/ Opensource project by Tung Nguyen Thanh
/ 2007
*/

#pragma once
#include <wx/treectrl.h>
#include <Capstone/headers/capstone.h>

class FMIP_TreeCtrl :public wxTreeCtrl
{
public:
	FMIP_TreeCtrl(wxWindow*, wxWindowID, long);
	void OnRightClick(wxTreeEvent&);
	void OnRefresh(wxMenuEvent&);
	//void OnKeyDown(wxTreeEvent& event);
	void OnPopupClick(wxCommandEvent& event);
#ifdef _WIN64
	WXLRESULT MSWWindowProc(WXUINT message, WXWPARAM wParam, WXLPARAM lParam);
#endif _WIN64
};	