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
	DWORD m_dwInitialHScrollBarState;
	BOOL m_blHScrollBarVisible;
	int m_intLastCheckFirstVisibleLine = 0;
public:
	FMIP_TextCtrl(VMContentDisplay*);
	//BOOL CanScroll(HWND, LONG);
	WXLRESULT MSWWindowProc(WXUINT message, WXWPARAM wParam, WXLPARAM lParam);
};