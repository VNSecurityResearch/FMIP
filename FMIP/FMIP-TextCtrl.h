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
	BOOL m_blInWindows10;
	DWORD m_dwInitialScrollBarState;
	int m_intLastCheckFirstVisibleLine = 0;
public:
	FMIP_TextCtrl(VMContentDisplay*);
	//BOOL CanScroll(HWND, LONG);
	BOOL IsWindows10();
	WXLRESULT MSWWindowProc(WXUINT message, WXWPARAM wParam, WXLPARAM lParam);
};