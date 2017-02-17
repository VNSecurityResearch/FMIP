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
 This file defines class VMContentDisplay - the dialog with the customized text control,
 in which, content of a VM region is outputted to.  
*/

#pragma once
#include "MainWindow.h"
#include <wx/dialog.h>
#include <Capstone/headers/capstone.h>

enum OUTPUT_TYPE
{
	OUTPUT_TYPE_STRING,
	OUTPUT_TYPE_ASM
};

class FMIP_TextCtrl;

class VMContentDisplay :
	public wxDialog
{
	friend class ThreadingOutputVMContent;
	//friend class FMIP_TextCtrl;
private:
	wxStatusBar* m_ptrStatusBar;
	ThreadingOutputVMContent* m_ptrAttachedThread = nullptr;
	FMIP_TextCtrl* m_ptrTextCtrl = nullptr;
	wxButton* m_ptrButtonOK;
	FMIP_TreeCtrl* m_ptrTreeCtrl;
	HWND m_hWndButtonOK;
	OUTPUT_TYPE m_OutputType;
public:
	VMContentDisplay(MainWindow*, FMIP_TreeCtrl*, OUTPUT_TYPE, const wxString& Title = wxT(""), wxWindowID = wxID_ANY);
	~VMContentDisplay();
	ThreadingOutputVMContent* GetAttachedThread();
	bool IsAttachedThreadCompleted();	
	void AttachThread(ThreadingOutputVMContent* pThread);
	void AppendOutput(const wxString& Text);
	wxString GetStatusText();
	void OnAttachedThreadComplete(wxThreadEvent& evt);
	void OnAttachedThreadStart(wxThreadEvent& evt);
	void OnClose(wxCloseEvent& evt);
	void OnOKClick(wxCommandEvent&);
	void OnLeftMouseUp(wxMouseEvent& MouseEvt);
	wxCriticalSection m_ptrAttachedThreadLocker;
	WXLRESULT MSWWindowProc(WXUINT message, WXWPARAM wParam, WXLPARAM lParam);
};

