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
  This file defines class ThreadingOutputVMContent - it implements a thread to output content of a VM region.
  The purpose of this seperate thread is not to make the GUI (belonging to the primary thread) freezed even if the task takes a long time to complete.
 */

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
	DWORD m_dwPId;
	HANDLE m_hProcessToRead;
	cs_mode m_CSMode;
public:
	ThreadingOutputVMContent(VMContentDisplay* pOutput);
	wxThread::ExitCode Entry() wxOVERRIDE;
	~ThreadingOutputVMContent();
};