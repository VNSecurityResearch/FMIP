/*
/ Opensource project by Tung Nguyen Thanh
/ 2007
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
	wxPanel* pPanel = new wxPanel(this);
	pPanel->SetBackgroundColour(wxColor(255, 255, 255));
	wxBoxSizer* pPanelSizer = new wxBoxSizer(wxVERTICAL);
	wxStaticBoxSizer* pStaticBox1 = new wxStaticBoxSizer(wxVERTICAL, pPanel, wxT("FMIP"));
	pStaticBox1->Add(new wxStaticText(pStaticBox1->GetStaticBox(), wxID_ANY, wxT("Version βeta\n3-42-V\n2016")));
	wxStaticBoxSizer* pStaticBox2 = new wxStaticBoxSizer(wxVERTICAL, pPanel, wxT("Các thành phần mã nguồn mở sử dụng trong chương trình:"));
	pStaticBox2->Add(new wxStaticText(pStaticBox2->GetStaticBox(), wxID_ANY, wxT("Memory strings (Process Hacker)\nCapstone")));
	pPanelSizer->Add(pStaticBox1, 1, wxGROW);
	pPanelSizer->Add(pStaticBox2, 1, wxEXPAND);
	pPanel->SetSizer(pPanelSizer);
	InnerTopVSizer->Add(pPanel, 1, wxALL | wxEXPAND);
	OutmostVSizer->Add(InnerTopVSizer, 1, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 3);
	m_ptrButtonOK = new wxButton(this, wxID_OK);
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