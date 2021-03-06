﻿/*
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
 GUI class:
 This file implements class MIPF_TextCtrl - the customized text control in VMContentDisplay.
*/

#include "wx/wxprec.h"
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include "MIPF-TextCtrl.h"

#ifdef __WXMSW__
#include <wx/msw/msvcrt.h>      // redefines the new() operator 
#endif

//BEGIN_EVENT_TABLE(MIPF_TextCtrl, wxTextCtrl)
//	EVT_CONTEXT_MENU(MIPF_TextCtrl::OnContextMenu)
//END_EVENT_TABLE()

MIPF_TextCtrl::MIPF_TextCtrl(VMContentDisplay* ptrParent) :wxTextCtrl(ptrParent, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
	wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2 | wxTE_DONTWRAP | wxTE_PROCESS_ENTER)
{
	m_ptrVMContentDisplay = ptrParent;
	m_ScrollInfo.cbSize = sizeof(SCROLLINFO);
	m_ScrollBarInfo.cbSize = sizeof(SCROLLBARINFO);
	m_ScrollInfo.fMask = SIF_RANGE | SIF_TRACKPOS | SIF_PAGE | SIF_POS;
	::GetScrollBarInfo(m_hWnd, OBJID_HSCROLL, &m_ScrollBarInfo);
	m_blHScrollBarVisible = m_ScrollBarInfo.rgstate[0] == 0 ? TRUE : FALSE;
	Bind(wxEVT_CONTEXT_MENU, &MIPF_TextCtrl::OnContextMenu, this);
}

void MIPF_TextCtrl::AppendText(const wxString & Text)
{
	SetInsertionPointEnd();
	// first, ensure that the new text will be in the default style
	long start, end;
	GetSelection(&start, &end);
	SetStyle(start, end, m_defaultStyle);
	::SendMessage(GetHWND(), EM_REPLACESEL, 0, (LPARAM)Text.t_str());
}

void MIPF_TextCtrl::OnPopupClick(wxCommandEvent & event)
{
	wxLogDebug(L"Popup clicked");
	event.Skip();
}

//BOOL MIPF_TextCtrl::CanScroll(HWND hWnd, LONG Direction)
//{
//	SCROLLBARINFO ScrollBarInfo;
//	ScrollBarInfo.cbSize = sizeof(SCROLLBARINFO);
//	::GetScrollBarInfo(hWnd, OBJID_HSCROLL, &ScrollBarInfo);
//	return (ScrollBarInfo.rgstate[0] != STATE_SYSTEM_INVISIBLE && ScrollBarInfo.rgstate[0] != STATE_SYSTEM_UNAVAILABLE) ? TRUE : FALSE;
//}

WXLRESULT MIPF_TextCtrl::MSWWindowProc(WXUINT nMsg, WXWPARAM wParam, WXLPARAM lParam)
{
	switch (nMsg)
	{
	case WM_SIZE:
		::GetScrollBarInfo(m_hWnd, OBJID_HSCROLL, &m_ScrollBarInfo);
		if (m_ScrollBarInfo.rgstate[0] == 0)
		{
			m_blHScrollBarVisible = TRUE;
			m_intLastCheckFirstVisibleLine = ::SendMessage(m_hWnd, EM_GETFIRSTVISIBLELINE, 0, 0);
		}
		break;

	case WM_PAINT:
		/*
		 In Windows 8 (or other version older than Windows 10), when the window is sized such that
		 the state of the scroll bar changes to STATE_SYSTEM_UNAVAILABLE, the scroll bar automatically scrolls to the top.
		 In that case, this is to restore the scroll bar to it's previous position.
		*/
		if (m_blHScrollBarVisible)
		{
			::GetScrollBarInfo(m_hWnd, OBJID_HSCROLL, &m_ScrollBarInfo);
			m_blHScrollBarVisible = m_ScrollBarInfo.rgstate[0] == 0 ? TRUE : FALSE;
			if (m_ScrollBarInfo.rgstate[0] == STATE_SYSTEM_UNAVAILABLE)
			{
				wxTextCtrl::MSWWindowProc(nMsg, wParam, lParam);
				int intFirstVisibleLine = ::SendMessage(m_hWnd, EM_GETFIRSTVISIBLELINE, 0, 0);
				if (intFirstVisibleLine != m_intLastCheckFirstVisibleLine)
					if ((::GetAsyncKeyState(VK_HOME) & 0x8000) != 0x8000)
						::SendMessage(m_hWnd, EM_LINESCROLL, 0, m_intLastCheckFirstVisibleLine - intFirstVisibleLine);

				return 0;
			}
		}
		break;

	case WM_KEYDOWN:
	{
		if ((::GetAsyncKeyState(VK_SHIFT) & 0x8000) == 0x8000) // Key SHIFT pressed...
			break; // ...just let the default procedure handle this
		switch (wParam)
		{
		case VK_UP:
			::SendMessage(m_hWnd, WM_VSCROLL, SB_LINEUP, NULL);
			return 0;

		case VK_DOWN:
			::SendMessage(m_hWnd, WM_VSCROLL, SB_LINEDOWN, NULL);
			return 0;

		case VK_LEFT:
			::SendMessage(m_hWnd, WM_HSCROLL, SB_LINELEFT, NULL);
			return 0;

		case VK_RIGHT:
			::SendMessage(m_hWnd, WM_HSCROLL, SB_LINERIGHT, NULL);
			return 0;

		case VK_PRIOR:
			::SendMessage(m_hWnd, WM_VSCROLL, SB_PAGEUP, NULL);
			return 0;

		case VK_NEXT:
			::SendMessage(m_hWnd, WM_VSCROLL, SB_PAGEDOWN, NULL);
			return 0;

		case VK_HOME:
			::SendMessage(m_hWnd, WM_VSCROLL, SB_TOP, NULL);
			::SendMessage(m_hWnd, WM_HSCROLL, SB_LEFT, NULL);
			return 0;

		case VK_END:
			::SendMessage(m_hWnd, WM_VSCROLL, SB_BOTTOM, NULL);
			::SendMessage(m_hWnd, WM_HSCROLL, SB_LEFT, NULL);
			return 0;

		default:
			break;
		}
	}
	break;

	case WM_HSCROLL:
	{
		switch (LOWORD(wParam))
		{
		case SB_LINERIGHT:
		{
			/*
			 In Windows 8 (or other version older than Windows 10), the scroll bar can scroll past the max scrolling position.
			 In that case, this is to keep the scroll bar postion within the correct scrolling range.
			*/
			::GetScrollInfo(m_hWnd, SB_HORZ, &m_ScrollInfo);
			UINT uintMaxScrlPosInWinOlderThan10 = m_ScrollInfo.nMax - (m_ScrollInfo.nPage - 1);
			UINT uintMaxScrlPosInWin10 = m_ScrollInfo.nMax - (m_ScrollInfo.nPage);
			if (m_ScrollInfo.nPos == uintMaxScrlPosInWinOlderThan10 || m_ScrollInfo.nPos == uintMaxScrlPosInWin10)
				return 0;
			//wxLogDebug(wxString::Format(wxT("npos: %d"), m_ScrollInfo.nPos));
			break;
		}

		case SB_PAGERIGHT:
		{
			/*
			 In Windows 8 (or other version older than Windows 10), the scroll bar can scroll past the max scrolling position.
			 In that case, this is to keep the scroll bar postion within the correct scrolling range.
			*/
			::GetScrollInfo(m_hWnd, SB_HORZ, &m_ScrollInfo);
			UINT uintScrBarMaxPosInSafeCase = m_ScrollInfo.nMax - (m_ScrollInfo.nPage);
			if (m_ScrollInfo.nPos + m_ScrollInfo.nPage <= uintScrBarMaxPosInSafeCase)
				break;
			else
				while ((UINT)m_ScrollInfo.nPos < uintScrBarMaxPosInSafeCase)
				{
					::SendMessage(m_hWnd, WM_HSCROLL, SB_LINERIGHT, NULL);
					::GetScrollInfo(m_hWnd, SB_HORZ, &m_ScrollInfo);
				}
			return 0;
		}

		default:
			break;
		}
		break;
	}

	/*case WM_RBUTTONUP:
	{
		m_ptrPopupMenu = new wxMenu;
		m_ptrPopupMenu->Append(COPY, wxT("&Copy"));
		m_ptrPopupMenu->Append(SELECT_ALL, wxT("&Select All"));
		m_ptrPopupMenu->Bind(wxEVT_COMMAND_MENU_SELECTED, &MIPF_TextCtrl::OnPopupClick, this);
		PopupMenu(m_ptrPopupMenu);
	}
		return 0;*/

	default:
		break;
	}
	WXLRESULT wxLResult = wxTextCtrl::MSWWindowProc(nMsg, wParam, lParam);// CallWindowProc((WNDPROC)m_oldWndProc, m_hWnd, nMsg, wParam, lParam);
	::HideCaret(m_hWnd);
	return wxLResult;
}

enum POPUP_MENU_ITEM
{
	COPY,
	SELECT_ALL,
};

void MIPF_TextCtrl::OnContextMenu(wxContextMenuEvent & event)
{
	wxMenu PopupMenu;// = new wxMenu;
	PopupMenu.Append(COPY, wxT("&Copy"));
	PopupMenu.Append(SELECT_ALL, wxT("&Select All"));
	PopupMenu.Bind(wxEVT_COMMAND_MENU_SELECTED, &MIPF_TextCtrl::OnPopupClick, this);
	wxWindowBase::PopupMenu(&PopupMenu);
}