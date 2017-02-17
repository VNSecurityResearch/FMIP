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