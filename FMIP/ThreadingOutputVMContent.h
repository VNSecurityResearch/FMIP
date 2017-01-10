#pragma once
#include <wx/thread.h>
#include <Capstone/headers/capstone.h>
#include "VMContentDisplay.h"

wxDEFINE_EVENT(EVT_THREAD_STARTED, wxThreadEvent);
wxDEFINE_EVENT(EVT_THREAD_COMPLETED, wxThreadEvent);

class ThreadingOutputVMContent : public wxThread
{
private:
	VMContentDisplay* m_ptrVMContentDisplay;
	DWORD m_ptrId;
	HANDLE m_hProcessToRead;
	cs_mode m_CSMode;
public:
	ThreadingOutputVMContent(VMContentDisplay* pOutput);
	wxThread::ExitCode Entry() wxOVERRIDE;
	~ThreadingOutputVMContent();
};