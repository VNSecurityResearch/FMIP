/*
/ Opensource project by Tung Nguyen Thanh
/ 2007
*/

#pragma once
#include <wx/textctrl.h>
#include "VMContentDisplay.h"

class FMIP_TextCtrl:public wxTextCtrl
{
private:
	VMContentDisplay* m_ptrVMContentDisplay;
	HWND m_hWndThis = nullptr;
	SCROLLINFO m_ScrollInfo;
	SCROLLBARINFO m_ScrollBarInfo;
	BOOL m_blHScrollBarVisible;
	int m_intLastCheckFirstVisibleLine = 0;
public:
	FMIP_TextCtrl(VMContentDisplay*);
	void AppendText(const wxString& Text);
	//BOOL CanScroll(HWND, LONG);
	WXLRESULT MSWWindowProc(WXUINT message, WXWPARAM wParam, WXLPARAM lParam);
};