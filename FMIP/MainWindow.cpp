#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include "MainWindow.h"
#include "About.h"
#include "FMIP.h"
#ifdef __WXMSW__
#include <wx/msw/msvcrt.h>      // redefines the new() operator 
#endif

DWORD WINAPI ManageX64Process(LPVOID lpParam)
{
	TCHAR szFileNameWithPath[MAX_PATH];
	GetTempPath(_countof(szFileNameWithPath), szFileNameWithPath);
	HRSRC hRsc = ::FindResource(nullptr, MAKEINTRESOURCE(109), L"EXE");
	HGLOBAL hExeRes = LockResource(LoadResource(NULL, hRsc));
	wcscat_s(szFileNameWithPath, L"FMIP-X64.exe");
	HANDLE hFile = INVALID_HANDLE_VALUE;
	BOOL bFileWritten = FALSE;
	{
		hFile = CreateFile(szFileNameWithPath,
			GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_DELETE | FILE_SHARE_READ,
			NULL,
			CREATE_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,
			NULL);
		if (hFile == INVALID_HANDLE_VALUE)
		{
			return 0;
		}

		HMODULE hInst = ::GetModuleHandle(nullptr);
		DWORD dwRscSize = SizeofResource(hInst, hRsc);
		DWORD dwNumberOfBytesWritten = 0;
		bFileWritten = WriteFile(hFile, (LPCVOID)hExeRes, dwRscSize, &dwNumberOfBytesWritten, NULL);
	}
	{
		if (hFile != INVALID_HANDLE_VALUE)
		{
			CloseHandle(hFile);
		}

		if (!bFileWritten) // can not write file
		{
			return 0;
		}
	}
	STARTUPINFO SI;
	::ZeroMemory(&SI, sizeof(SI));
	SI.cb = sizeof(SI);
	PROCESS_INFORMATION PI;
	::ZeroMemory(&PI, sizeof(PI));
	::CreateProcess(szFileNameWithPath, nullptr, nullptr, nullptr, FALSE, 0, nullptr, nullptr, &SI, &PI);
	::WaitForSingleObject(PI.hProcess, INFINITE);
	::DeleteFile(szFileNameWithPath);
	::CloseHandle(PI.hProcess);
	::CloseHandle(PI.hThread);
	::ExitProcess(0);
	//::SendMessage(::FindWindow(nullptr, AppTitleWoW), CM_TERMINATE, 0, 0);
	return 1;
}

MainWindow::MainWindow(const wxString& Title) : wxFrame(nullptr, wxID_ANY, Title, wxDefaultPosition, wxSize(550, 640))
{
#ifndef _WIN64
	if (FMIP::IsProcessWoW64(GetCurrentProcess()) == TRUE)
	{
		SetTitle(AppTitleWoW);
		CreateThread(
			NULL,                   // default security attributes
			0,                      // use default stack size  
			ManageX64Process,       // thread function name
			nullptr,          // argument to thread function 
			0,                      // use default creation flags 
			nullptr);
		return;
	}
#endif
	wxMenu* ptrMenuRefresh = new wxMenu;
	wxMenu* ptrMenuAbout = new wxMenu;
	wxMenuBar* ptrMenuBar = new wxMenuBar;
	ptrMenuBar->Append(ptrMenuRefresh, wxT("&Quét lại"));
	ptrMenuBar->Append(ptrMenuAbout, wxT("&Thông tin"));
	SetMenuBar(ptrMenuBar);
	wxBoxSizer* ptrVBox = new wxBoxSizer(wxVERTICAL);
	m_ptrFMIP_TreeCtrl = new FMIP_TreeCtrl(this, wxID_ANY, wxTR_DEFAULT_STYLE | wxTR_HIDE_ROOT | wxTR_FULL_ROW_HIGHLIGHT | wxTR_NO_LINES | wxTR_TWIST_BUTTONS);
	ptrMenuAbout->Bind(wxEVT_MENU_OPEN, &MainWindow::OnAbout, this);
	ptrMenuRefresh->Bind(wxEVT_MENU_OPEN, &FMIP_TreeCtrl::OnRefresh, m_ptrFMIP_TreeCtrl);
	CreateStatusBar(1, wxSTB_DEFAULT_STYLE, wxID_ANY);
	m_ptrFMIP_TreeCtrl->OnRefresh(wxMenuEvent()); //wxQueueEvent(pMenuRefresh, new wxMenuEvent(wxEVT_MENU_OPEN));
	m_ptrFMIP_TreeCtrl->Bind(wxEVT_TREE_ITEM_RIGHT_CLICK, &FMIP_TreeCtrl::OnRightClick, m_ptrFMIP_TreeCtrl);
	//m_ptrFMIP_TreeCtrl->Bind(wxEVT_TREE_KEY_DOWN, &FMIP_TreeCtrl::OnKeyDown, m_ptrFMIP_TreeCtrl);
	ptrVBox->Add(m_ptrFMIP_TreeCtrl, 1, wxEXPAND);
	SetSizer(ptrVBox);
	Show(true);
}

void MainWindow::OnAbout(wxMenuEvent& event)
{
#define AboutTitle wxT("Giới thiệu")
	POINT pntCursorPos;
	::GetCursorPos(&pntCursorPos);
	About* ptrAbout = static_cast<About*>(wxWindow::FindWindowByLabel(AboutTitle, this));
	if (ptrAbout != nullptr)
	{
		ptrAbout->SetPosition(wxPoint(pntCursorPos.x, pntCursorPos.y));
		ptrAbout->SetFocus();
		return;
	}
	ptrAbout = new About(this, wxID_ANY, AboutTitle, pntCursorPos);
	ptrAbout->Show();
}

WXLRESULT MainWindow::MSWWindowProc(WXUINT message, WXWPARAM wParam, WXLPARAM lParam)
{
	// this is the case only with the WoW64 X86 version to handle a request from the X64 version for querying virtual memory of a X86 process
	switch (message)
	{
	case WM_COPYDATA:
	{
		COPYDATASTRUCT* ptrCopyDataStruct = (COPYDATASTRUCT*)lParam;
		TREE_ITEM_PROPERTIES* ptrNodeProperty = (TREE_ITEM_PROPERTIES*)ptrCopyDataStruct->lpData;
		switch (ptrCopyDataStruct->dwData)
		{
		case ACTION::ACTION_REQUEST_FOR_X86HANDLING:
		{
			DWORD dwPid = ptrNodeProperty->PROCESSNAMEPID.dwPId;
			HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, dwPid);
			if ((hProcess != INVALID_HANDLE_VALUE) && (hProcess != nullptr))
			{
				FMIP::MakeTreeNodesForRemoteProcess((HWND)wParam, hProcess, ptrNodeProperty->PROCESSNAMEPID);
			}
			::CloseHandle(hProcess);
			break;
		}

		default:
			break;
		} // close switch (ptrCopyDataStruct->dwData)
		return 0;
	}

	// this is to get rid of the idle state when a menu of the main window is clicked
	case WM_ENTERIDLE:
	{
		INPUT input;
		WORD vkey = VK_ESCAPE;
		input.type = INPUT_KEYBOARD;
		input.ki.wScan = MapVirtualKey(vkey, MAPVK_VK_TO_VSC);
		input.ki.time = 0;
		input.ki.dwExtraInfo = 0;
		input.ki.wVk = vkey;
		input.ki.dwFlags = 0; // there is no KEYEVENTF_KEYDOWN
		::SendInput(1, &input, sizeof(INPUT));
		input.ki.dwFlags = KEYEVENTF_KEYUP;
		::SendInput(1, &input, sizeof(INPUT));
		return 0;
	}

	case CM_TERMINATE:
	{
		::PostQuitMessage(0);
	}

	case WM_NOTIFY:
	{
		WXLRESULT Result = CDRF_DODEFAULT;
		if (m_ptrFMIP_TreeCtrl == nullptr) break;
		if (((LPNMHDR)lParam)->hwndFrom == m_ptrFMIP_TreeCtrl->GetHWND())
		{
			if (((LPNMHDR)lParam)->code == NM_CUSTOMDRAW)
			{
				LPNMTVCUSTOMDRAW lpNMCustomDraw = (LPNMTVCUSTOMDRAW)lParam;
				switch (lpNMCustomDraw->nmcd.dwDrawStage)
				{
				case CDDS_PREPAINT:
				{
					return CDRF_NOTIFYITEMDRAW;
				}

				case CDDS_ITEMPREPAINT:
				{
					Generic_Tree_Item* ptrGenericTreeItem = static_cast<Generic_Tree_Item*>(m_ptrFMIP_TreeCtrl->GetItemData((wxTreeItemId*)lpNMCustomDraw->nmcd.dwItemSpec));
					if (ptrGenericTreeItem != nullptr)
					{
						if (ptrGenericTreeItem->IsRedWarning() == TRUE)
						{
							lpNMCustomDraw->clrText = RGB(255, 0, 0);
						}
						else
						{
							lpNMCustomDraw->clrText = CLR_DEFAULT;
						}
						if (ptrGenericTreeItem->GetType() != TREE_ITEM_TYPE::TREE_ITEM_TYPE_PROCESS_NAME_PID)
						{
							wxFont fntSubItem (wxFontInfo().FaceName("Consolas"));
							SelectObject(lpNMCustomDraw->nmcd.hdc, fntSubItem.GetHFONT());
							Result |= CDRF_NEWFONT;
						}
					}
					return Result;
				}

				default:
					break;
				}
			}
		}
	}

	default:
		break;
	}
	return wxFrame::MSWWindowProc(message, wParam, lParam);
}