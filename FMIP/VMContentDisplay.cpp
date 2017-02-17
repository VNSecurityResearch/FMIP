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

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include "FMIP.h"
#include "ThreadingOutputVMContent.h"
#include "FMIP-TextCtrl.h"
#ifdef __WXMSW__
#include <wx/msw/msvcrt.h>      // redefines the new() operator 
#endif

VMContentDisplay::VMContentDisplay(MainWindow* Parent, FMIP_TreeCtrl* ptrTreeCtrl, OUTPUT_TYPE OutputType, const wxString& Title, wxWindowID WindowID)
	:wxDialog(Parent, WindowID, Title, wxDefaultPosition, wxSize(550, 640), wxDEFAULT_DIALOG_STYLE | wxDIALOG_NO_PARENT | wxMAXIMIZE_BOX | wxMINIMIZE_BOX | wxRESIZE_BORDER), m_ptrTreeCtrl(ptrTreeCtrl)
{
	m_OutputType = OutputType;
	wxBoxSizer* pVBox = new wxBoxSizer(wxVERTICAL);
	this->m_ptrTextCtrl = new FMIP_TextCtrl(this);
	wxTextAttr TextAttr;
	TextAttr.SetFontFaceName("Consolas");
	//TextAttr.SetFont(wxFontInfo().FaceName("Consolas"));
	SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_MENU));
	m_ptrTextCtrl->SetDefaultStyle(TextAttr);
	m_ptrTextCtrl->Bind(wxEVT_LEFT_UP, &VMContentDisplay::OnLeftMouseUp, this);
	pVBox->Add(m_ptrTextCtrl, 1, wxLEFT | wxRIGHT | wxTOP | wxEXPAND, 3);
	m_ptrButtonOK = new wxButton(this, wxID_OK);
	m_hWndButtonOK = m_ptrButtonOK->GetHWND();
	pVBox->Add(m_ptrButtonOK, 0, wxALIGN_RIGHT | wxALL, 10);
	m_ptrStatusBar = new wxStatusBar(this);
	pVBox->Add(m_ptrStatusBar, 0, wxGROW);
	SetSizer(pVBox);
	Bind(wxEVT_CLOSE_WINDOW, &VMContentDisplay::OnClose, this);
	m_ptrButtonOK->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &VMContentDisplay::OnOKClick, this);
	m_ptrButtonOK->Bind(wxEVT_LEFT_UP, &VMContentDisplay::OnLeftMouseUp, this);
	//Layout(); when this needed?
	this->Show(true);
	ThreadingOutputVMContent* pThreadingOutputVMContent = new ThreadingOutputVMContent(this);
	if (pThreadingOutputVMContent->Run() != wxTHREAD_NO_ERROR)
	{
		OutputDebugString(L"Could not create thread!");
	}
}

VMContentDisplay::~VMContentDisplay()
{
	wxCriticalSectionLocker CSLocker(m_ptrAttachedThreadLocker);
	if ((m_ptrAttachedThread != nullptr))
	{
		m_ptrAttachedThread->Delete(nullptr, wxTHREAD_WAIT_BLOCK /*wxTHREAD_WAIT_YIELD*/); // check this if program freezes when the thread has not finished
	}
}

ThreadingOutputVMContent* VMContentDisplay::GetAttachedThread()
{
	wxCriticalSectionLocker CSLocker(m_ptrAttachedThreadLocker);
	return m_ptrAttachedThread;
}

bool VMContentDisplay::IsAttachedThreadCompleted()
{
	wxCriticalSectionLocker CSLocker(m_ptrAttachedThreadLocker);
	return (m_ptrAttachedThread == nullptr ? true : false);
}

void VMContentDisplay::AttachThread(ThreadingOutputVMContent* ptrThread)
{
	wxCriticalSectionLocker CSLocker(m_ptrAttachedThreadLocker);
	m_ptrAttachedThread = ptrThread;
}

void VMContentDisplay::AppendOutput(const wxString& Text)
{
	m_ptrTextCtrl->AppendText(Text);
	m_ptrTextCtrl->ShowPosition(1);
	
}

wxString VMContentDisplay::GetStatusText()
{
	wxString strStatusText = m_ptrStatusBar->GetStatusText(); // this->GetStatusBar()->GetStatusText();
	assert(strStatusText != "");
	return strStatusText;
}

void VMContentDisplay::OnAttachedThreadComplete(wxThreadEvent& evt)
{
	//wxLogDebug(wxT("Thread completed"));
}

void VMContentDisplay::OnAttachedThreadStart(wxThreadEvent& evt)
{
}

void VMContentDisplay::OnClose(wxCloseEvent& evt)
{
	this->Destroy();
}

void VMContentDisplay::OnOKClick(wxCommandEvent& WXUNUSED(evt))
{
	this->Destroy();
}

void VMContentDisplay::OnLeftMouseUp(wxMouseEvent& MouseEvt)
{
	m_ptrButtonOK->SetDefault();
	MouseEvt.Skip();
}

WXLRESULT VMContentDisplay::MSWWindowProc(WXUINT message, WXWPARAM wParam, WXLPARAM lParam)
{
	switch (message)
	{
	case WM_ACTIVATE:
		if (LOWORD(wParam) != WA_INACTIVE)
			m_ptrButtonOK->SetDefault(); // = ::SendMessage(m_ptrButtonOK->GetHWND(), BM_SETSTYLE, BS_DEFPUSHBUTTON | BS_TEXT, TRUE);
		else
			::SendMessage(m_hWndButtonOK, BM_SETSTYLE, BS_PUSHBUTTON | BS_TEXT, TRUE);
		break;

	default:
		break;
	}
	return wxDialog::MSWWindowProc(message, wParam, lParam);
}