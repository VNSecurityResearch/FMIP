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
	WXLRESULT MSWWindowProc(WXUINT message, WXWPARAM wParam, WXLPARAM lParam);
};