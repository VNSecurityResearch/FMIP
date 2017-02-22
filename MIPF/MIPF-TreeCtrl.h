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
 This file declares class MIPF_TreeCtrl - the customized tree control of this program.
*/

#pragma once
#include <wx/treectrl.h>
#include <Capstone/headers/capstone.h>

class MIPF_TreeCtrl :public wxTreeCtrl
{
public:
	MIPF_TreeCtrl(wxWindow*, wxWindowID, long);
	void OnRightClick(wxTreeEvent&);
	void OnRescan(wxMenuEvent&);
	//void OnKeyDown(wxTreeEvent& event);
	void OnPopupClick(wxCommandEvent& event);
#ifdef _WIN64
	WXLRESULT MSWWindowProc(WXUINT message, WXWPARAM wParam, WXLPARAM lParam);
#endif _WIN64
};	