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
 This file defines class FMIP_TextCtrl - the customized text control of this program.
*/

#pragma once
#include <wx/textctrl.h>
#include "VMContentDisplay.h"

class FMIP_TextCtrl :public wxTextCtrl
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