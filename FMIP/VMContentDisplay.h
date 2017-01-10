#pragma once
#include "MainWindow.h"
#include <wx/dialog.h>
#include <Capstone/headers/capstone.h>

enum OUTPUT_TYPE
{
	OUTPUT_TYPE_STRING,
	OUTPUT_TYPE_ASM
};


class VMContentDisplay :
	public wxDialog
{
	friend class ThreadingOutputVMContent;
	friend class FMIP_TextCtrl;
private:
	wxStatusBar* m_ptrStatusBar;
	ThreadingOutputVMContent* m_ptrAttachedThread = nullptr;
	FMIP_TextCtrl* m_ptrTextCtrl = nullptr;
	//HWND m_hwndTextCtrl = nullptr;
	wxButton* m_ptrButtonOK;
	FMIP_TreeCtrl* m_ptrTreeCtrl;
	/*DWORD m_ptrId;
	HANDLE m_hProcessToRead;
	cs_mode m_CSMode;*/
	HWND m_hWndButtonOK;
	OUTPUT_TYPE m_OutputType;
public:
	VMContentDisplay(MainWindow*, FMIP_TreeCtrl*, OUTPUT_TYPE, const wxString& Title = wxT(""), wxWindowID = wxID_ANY);
	~VMContentDisplay();
	ThreadingOutputVMContent* GetAttachedThread();
	bool IsAttachedThreadCompleted();	
	void AttachThread(ThreadingOutputVMContent* pThread);
	void Output(const wxString& Text);
	wxString GetStatusText();
	void OnAttachedThreadComplete(wxThreadEvent& evt);
	void OnAttachedThreadStart(wxThreadEvent& evt);
	void OnClose(wxCloseEvent& evt);
	void OnOKClick(wxCommandEvent&);
	wxCriticalSection m_ptrAttachedThreadLocker;
	WXLRESULT MSWWindowProc(WXUINT message, WXWPARAM wParam, WXLPARAM lParam);
};

