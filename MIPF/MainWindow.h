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
 This file declares class MainWindow - the main window of this program.
*/

#pragma once
#include <wx/frame.h>
#include "MIPF-TreeCtrl.h"

class MainWindow :public wxFrame
{
private:
	MIPF_TreeCtrl* m_ptrMIPF_TreeCtrl = nullptr;
public:
	MainWindow(const wxString&);
	void OnAbout(wxMenuEvent& event);
	WXLRESULT MSWWindowProc(WXUINT message, WXWPARAM wParam, WXLPARAM lParam);
};