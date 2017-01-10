#include "wx/wxprec.h"
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include "FMIP-TextCtrl.h"
#ifdef __WXMSW__
#include <wx/msw/msvcrt.h>      // redefines the new() operator 
#endif

FMIP_TextCtrl::FMIP_TextCtrl(VMContentDisplay* ptrParent) :wxTextCtrl(ptrParent, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
	wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2 | wxTE_DONTWRAP | wxTE_PROCESS_ENTER)
{
	m_ptrVMContentDisplay = ptrParent;
	m_hWndThis = this->GetHWND();
	m_ScrollInfo.cbSize = sizeof(SCROLLINFO);
	m_ScrollBarInfo.cbSize = sizeof(SCROLLBARINFO);
	m_ScrollInfo.fMask = SIF_RANGE | SIF_TRACKPOS | SIF_PAGE | SIF_POS;
	m_blInWindows10 = IsWindows10();
}

//BOOL TTTBTMÐTextCtrl::CanScroll(HWND hWnd, LONG Direction)
//{
//	SCROLLBARINFO ScrollBarInfo;
//	ScrollBarInfo.cbSize = sizeof(SCROLLBARINFO);
//	::GetScrollBarInfo(hWnd, OBJID_HSCROLL, &ScrollBarInfo);
//	return (ScrollBarInfo.rgstate[0] != STATE_SYSTEM_INVISIBLE && ScrollBarInfo.rgstate[0] != STATE_SYSTEM_UNAVAILABLE) ? TRUE : FALSE;
//}

BOOL FMIP_TextCtrl::IsWindows10()
{
	OSVERSIONINFOEX osvi;
	DWORDLONG dwlConditionMask = 0;
	int op = VER_GREATER_EQUAL;

	// Initialize the OSVERSIONINFOEX structure.
	ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	osvi.dwMajorVersion = 10;

	// Initialize the condition mask.
	VER_SET_CONDITION(dwlConditionMask, VER_MAJORVERSION, op);

	// Perform the test.
	return VerifyVersionInfo(
		&osvi,
		VER_MAJORVERSION,
		dwlConditionMask);
}

WXLRESULT FMIP_TextCtrl::MSWWindowProc(WXUINT nMsg, WXWPARAM wParam, WXLPARAM lParam)
{
	switch (nMsg)
	{
	case WM_PAINT:
		// If running in Windows 10, just let the default procedure handle this
		if (m_blInWindows10)
			break;

		// Prevents auto scrolling to the top when running in other versions than Windows 10
		::GetScrollBarInfo(m_hWndThis, OBJID_HSCROLL, &m_ScrollBarInfo);
		if (m_ScrollBarInfo.rgstate[0] == STATE_SYSTEM_UNAVAILABLE)
		{
			/*if (m_blAutoScrollToTop)
			{*/
			wxTextCtrl::MSWWindowProc(nMsg, wParam, lParam);
			//m_ScrollInfo.fMask = SIF_POS;
			//::GetScrollInfo(m_hWndThis, SB_VERT, &m_ScrollInfo);
			int intFirstVisibleLine = ::SendMessage(m_hWndThis, EM_GETFIRSTVISIBLELINE, 0, 0);
			if (intFirstVisibleLine != m_intLastCheckFirstVisibleLine)
				if ((::GetAsyncKeyState(VK_HOME) & 0x8000) != 0x8000)
					::SendMessage(m_hWndThis, EM_LINESCROLL, 0, m_intLastCheckFirstVisibleLine - intFirstVisibleLine);
			//m_blAutoScrollToTop = FALSE;
			return 0;
			//}
		}
		/*else
		{
			m_blAutoScrollToTop = TRUE;
		}*/
		break;
	case WM_VSCROLL:
		wxTextCtrl::MSWWindowProc(nMsg, wParam, lParam);
		m_intLastCheckFirstVisibleLine = ::SendMessage(m_hWndThis, EM_GETFIRSTVISIBLELINE, 0, 0);
		return 0;
	case WM_KEYDOWN:
	{
		if (::GetAsyncKeyState(VK_SHIFT))
			/*wxTextCtrl::MSWWindowProc(nMsg, wParam, lParam);
			::HideCaret(m_hWndThis);
			return 0;*/
			break;
		switch (wParam)
		{
		case VK_UP:
			::SendMessage(m_hWndThis, WM_VSCROLL, SB_LINEUP, NULL);
			break;
		case VK_DOWN:
			::SendMessage(m_hWndThis, WM_VSCROLL, SB_LINEDOWN, NULL);
			break;
		case VK_LEFT:
			/*if (!CanScroll(m_hWndThis, OBJID_HSCROLL))
				break;*/
				/*if (::GetAsyncKeyState(VK_CONTROL) & 0x8000)
					::SendMessage(m_hWndThis, WM_HSCROLL, SB_PAGELEFT, NULL);
				else*/
			::SendMessage(m_hWndThis, WM_HSCROLL, SB_LINELEFT, NULL);
			break;
		case VK_RIGHT:
			/*if (!CanScroll(m_hWndThis, OBJID_HSCROLL))
			break;*/
			//if (m_blInWindows10)
			//	::SendMessage(m_hWndThis, WM_HSCROLL, SB_LINERIGHT, NULL);
			//else
			//{
			//	//m_ScrollInfo.fMask = SIF_RANGE | SIF_TRACKPOS | SIF_PAGE | SIF_POS;
			//	::GetScrollInfo(m_hWndThis, SB_HORZ, &m_ScrollInfo);
			//	if (m_ScrollInfo.nTrackPos < m_ScrollInfo.nMax - (m_ScrollInfo.nPage - 1))
			//		::SendMessage(m_hWndThis, WM_HSCROLL, SB_LINERIGHT, NULL);
			//}
			::SendMessage(m_hWndThis, WM_HSCROLL, SB_LINERIGHT, NULL);
			break;
		case VK_PRIOR:
			::SendMessage(m_hWndThis, WM_VSCROLL, SB_PAGEUP, NULL);
			break;
		case VK_NEXT:
			::SendMessage(m_hWndThis, WM_VSCROLL, SB_PAGEDOWN, NULL);
			break;
		case VK_HOME:
			::SendMessage(m_hWndThis, WM_VSCROLL, SB_TOP, NULL);
			::SendMessage(m_hWndThis, WM_HSCROLL, SB_LEFT, NULL);
			break;
		case VK_END:
			::SendMessage(m_hWndThis, WM_VSCROLL, SB_BOTTOM, NULL);
			::SendMessage(m_hWndThis, WM_HSCROLL, SB_LEFT, NULL);
			break;
		default:
			break;
		}
		return 0;
	}
	case WM_HSCROLL:
	{
		//m_ScrollInfo.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
		switch (LOWORD(wParam))
		{
		case SB_LINERIGHT:
			if (!m_blInWindows10)
			{
				//m_ScrollInfo.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
				::GetScrollInfo(m_hWndThis, SB_HORZ, &m_ScrollInfo);
				if (m_ScrollInfo.nPos >= m_ScrollInfo.nMax - (m_ScrollInfo.nPage - 1))
					return 0;
			}
			else
				::SendMessage(m_hWndThis, WM_HSCROLL, SB_LINERIGHT, NULL);
			break;
		case SB_PAGERIGHT:
			if (m_blInWindows10)
				break;
			//m_ScrollInfo.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
			::GetScrollInfo(m_hWndThis, SB_HORZ, &m_ScrollInfo);
			if (m_ScrollInfo.nPos + m_ScrollInfo.nPage <= m_ScrollInfo.nMax - (m_ScrollInfo.nPage - 1))
				break;
			else
				while (m_ScrollInfo.nPos < m_ScrollInfo.nMax - (m_ScrollInfo.nPage - 1))
				{
					::SendMessage(m_hWndThis, WM_HSCROLL, SB_LINERIGHT, NULL);
					::GetScrollInfo(m_hWndThis, SB_HORZ, &m_ScrollInfo);
				}
			return 0;
		default:
			break;
		}
	}
	default:
		break;
	}
	WXLRESULT wxLResult = wxTextCtrl::MSWWindowProc(nMsg, wParam, lParam);
	::HideCaret(m_hWndThis);
	return wxLResult;
}