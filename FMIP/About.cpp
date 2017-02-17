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
 This file implements class About - the dialog displaying information about this program.
*/

#include "wx/wxprec.h"
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include "About.h"
#ifdef __WXMSW__
#include <wx/msw/msvcrt.h>      // redefines the new() operator 
#endif

About::About(MainWindow* Parent, wxWindowID WindowID, const wxString& Title, POINT pntCursorPos)
	:wxDialog(Parent, WindowID, Title, wxPoint(pntCursorPos.x, pntCursorPos.y), wxSize(386, 300), wxDEFAULT_DIALOG_STYLE)
{
	this->SetSizeHints(wxDefaultSize, wxDefaultSize);
	wxBoxSizer* OutmostVSizer;
	OutmostVSizer = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* InnerTopVSizer;
	InnerTopVSizer = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* InnerBottomVSizer;
	InnerBottomVSizer = new wxBoxSizer(wxVERTICAL);
	wxPanel* ptrPanel = new wxPanel(this);
	ptrPanel->Bind(wxEVT_LEFT_UP, &About::OnLeftMouseUp, this);
	ptrPanel->SetBackgroundColour(wxColor(255, 255, 255));
	wxBoxSizer* ptrPanelSizer = new wxBoxSizer(wxVERTICAL);
	wxStaticBoxSizer* ptrStaticBox1 = new wxStaticBoxSizer(wxVERTICAL, ptrPanel, wxT("FMIP"));
	ptrStaticBox1->Add(new wxStaticText(ptrStaticBox1->GetStaticBox(), wxID_ANY, wxT("Version 1.0.0.1\n3-42-V\n2016-2017")));
	wxStaticBoxSizer* pStaticBox2 = new wxStaticBoxSizer(wxVERTICAL, ptrPanel, wxT("Các thành phần mã nguồn mở sử dụng trong chương trình:"));
	pStaticBox2->Add(new wxStaticText(pStaticBox2->GetStaticBox(), wxID_ANY, wxT("Memory strings (Process Hacker)\nCapstone")));
	ptrPanelSizer->Add(ptrStaticBox1, 1, wxGROW);
	ptrPanelSizer->Add(pStaticBox2, 1, wxEXPAND);
	ptrPanel->SetSizer(ptrPanelSizer);
	InnerTopVSizer->Add(ptrPanel, 1, wxALL | wxEXPAND);
	OutmostVSizer->Add(InnerTopVSizer, 1, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 3);
	m_ptrButtonOK = new wxButton(this, wxID_OK);	
	m_ptrButtonOK->Bind(wxEVT_LEFT_UP, &About::OnLeftMouseUp, this);
	InnerBottomVSizer->Add(m_ptrButtonOK);
	m_ptrButtonOK->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &About::OnButtonClick, this);
	OutmostVSizer->Add(InnerBottomVSizer, 0, wxALL | wxALIGN_RIGHT, 10);
	this->SetSizer(OutmostVSizer);
	this->Layout();
	//this->Centre(wxLEFT | wxRIGHT);
	Bind(wxEVT_CLOSE_WINDOW, &About::OnClose, this);
}

void About::OnClose(wxCloseEvent& evt)
{
	this->Destroy();
}

void About::OnButtonClick(wxCommandEvent& evt)
{
	this->Destroy();
}

void About::OnLeftMouseUp(wxMouseEvent & evtMouse)
{
	m_ptrButtonOK->SetDefault();
	evtMouse.Skip();
}

WXLRESULT About::MSWWindowProc(WXUINT message, WXWPARAM wParam, WXLPARAM lParam)
{
	switch (message)
	{
	case WM_ACTIVATE:
		if (LOWORD(wParam) != WA_INACTIVE)
			m_ptrButtonOK->SetDefault(); // = ::SendMessage(m_ptrButtonOK->GetHWND(), BM_SETSTYLE, BS_DEFPUSHBUTTON | BS_TEXT, TRUE);
		else
			::SendMessage(m_ptrButtonOK->GetHWND(), BM_SETSTYLE, BS_PUSHBUTTON | BS_TEXT, TRUE);
		break;

	default:
		break;
	}
	return wxDialog::MSWWindowProc(message, wParam, lParam);
}