/*
/ Opensource project by Tung Nguyen Thanh
/ 2007
*/

#pragma once
#include <wx/frame.h>
#include "FMIP-TreeCtrl.h"

class MainWindow :public wxFrame
{
private:
	FMIP_TreeCtrl* m_ptrFMIP_TreeCtrl = nullptr;
public:
	MainWindow(const wxString&);
	void OnAbout(wxMenuEvent& event);
	WXLRESULT MSWWindowProc(WXUINT message, WXWPARAM wParam, WXLPARAM lParam);
};