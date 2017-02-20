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
  This file declares class About - the dialog displaying information about this program.
*/

#pragma once
#include "MainWindow.h"

class About : public wxDialog
{
private:
	wxButton* m_ptrButtonOK;
public:
	About(MainWindow*, wxWindowID, const wxString& , POINT);
	void OnClose(wxCloseEvent&);
	void OnButtonClick(wxCommandEvent&);
	void OnLeftMouseUp(wxMouseEvent& evtMouse);
	WXLRESULT MSWWindowProc(WXUINT message, WXWPARAM wParam, WXLPARAM lParam);
};