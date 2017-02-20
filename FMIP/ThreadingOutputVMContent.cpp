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
 This file implements class ThreadingOutputVMContent.
 This class outputs content of a VM region either in Assembly or in Strings to a window.
*/

#include "wx/wxprec.h"
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include "FMIP.h"
#include "ThreadingOutputVMContent.h"
#include "FMIP-TextCtrl.h"
#include <memory>

#ifdef __WXMSW__
#include <wx/msw/msvcrt.h>      // redefines the new() operator 
#endif

ThreadingOutputVMContent::ThreadingOutputVMContent(VMContentDisplay* pVMContentDisplay)
	:wxThread(wxTHREAD_DETACHED)
{
	m_ptrVMContentDisplay = pVMContentDisplay;
	m_ptrVMContentDisplay->Bind(EVT_THREAD_COMPLETED, &VMContentDisplay::OnAttachedThreadComplete, m_ptrVMContentDisplay);
	m_ptrVMContentDisplay->Bind(EVT_THREAD_STARTED, &VMContentDisplay::OnAttachedThreadStart, m_ptrVMContentDisplay);

}

ThreadingOutputVMContent::~ThreadingOutputVMContent()
{
	wxCriticalSectionLocker CSLocker(m_ptrVMContentDisplay->m_ptrAttachedThreadLocker);
	m_ptrVMContentDisplay->m_ptrAttachedThread = nullptr;
	wxQueueEvent(m_ptrVMContentDisplay, new wxThreadEvent(EVT_THREAD_COMPLETED));
	OutputDebugString(L"Destroy	ThreadingOutputVMContent\n");
}

wxThread::ExitCode ThreadingOutputVMContent::Entry()
{
	{
		wxCriticalSectionLocker CSLocker(m_ptrVMContentDisplay->m_ptrAttachedThreadLocker);
		m_ptrVMContentDisplay->m_ptrAttachedThread = this;
	}
	wxTreeItemId tiSelected = m_ptrVMContentDisplay->m_ptrTreeCtrl->GetSelection();
	wxString wxstrProcessNamePId;
	wxTreeItemId tiIdIterator = tiSelected;
	TREE_ITEM_TYPE TreeItemType;
	Generic_Tree_Item* ptrGenericTreeItem;
	do
	{
		tiIdIterator = m_ptrVMContentDisplay->m_ptrTreeCtrl->GetItemParent(tiIdIterator);
		ptrGenericTreeItem = static_cast<Generic_Tree_Item*>(m_ptrVMContentDisplay->m_ptrTreeCtrl->GetItemData(tiIdIterator));
		TreeItemType = ptrGenericTreeItem->GetType();
	} while (TreeItemType != PROCESS_NAME_WITH_PID);
	wxstrProcessNamePId = m_ptrVMContentDisplay->m_ptrTreeCtrl->GetItemText(tiIdIterator);
	//m_ptrVMContentDisplay->SetStatusText(strProcessNamePId);
	m_dwPId = static_cast<Tree_Item_Process_Name_PId*>(ptrGenericTreeItem)->GetPId();
	m_hProcessToRead = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, m_dwPId);
	if ((m_hProcessToRead == INVALID_HANDLE_VALUE) || (m_hProcessToRead == nullptr))
	{
		::MessageBox(m_ptrVMContentDisplay->GetHWND(),
			L"Không đọc được nội dung của tiến trình này. Thường là do tiến trình đã kết thúc. Hãy nhấn Quét lại trên trình đơn.", AppTitle, MB_ICONINFORMATION | MB_OK);
		return (wxThread::ExitCode)1;
	}
	wxString wxstrStatusText = wxstrProcessNamePId;// m_ptrVMContentDisplay->GetStatusText();
	m_ptrVMContentDisplay->m_ptrStatusBar->SetStatusText(wxstrStatusText.Append(wxT(" --> Đang xử lý...")));
	//wxTreeItemId SelectedTreeItemId = m_ptrVMContentDisplay->m_ptrTreeCtrl->GetSelection();
	wxTreeItemIdValue tiIdValue;
	//tiIdIterator = m_ptrVMContentDisplay->m_ptrTreeCtrl->GetFirstChild(tiSelected, tiIdValue);
	ptrGenericTreeItem = static_cast<Generic_Tree_Item*>(m_ptrVMContentDisplay->m_ptrTreeCtrl->GetItemData(tiSelected));
	TreeItemType = ptrGenericTreeItem->GetType();
	wxString wxstrTitle;
	LPCVOID pcvoidRegionStart = nullptr;
	LPCVOID pcvoidRegionEnd = nullptr;
	switch (TreeItemType)
	{
	case ALLOCATION_BASE:
		pcvoidRegionStart = static_cast<Tree_Item_Allocation_Base*>(m_ptrVMContentDisplay->m_ptrTreeCtrl->GetItemData(tiSelected))->GetAllocationBase();
		wxstrTitle.Append(wxstrTitle.Format(L"0x%p [+]", pcvoidRegionStart));
		tiIdIterator = m_ptrVMContentDisplay->m_ptrTreeCtrl->GetFirstChild(tiSelected, tiIdValue);
		break;
	case REGION:
		pcvoidRegionStart = static_cast<Tree_Item_Region*>(m_ptrVMContentDisplay->m_ptrTreeCtrl->GetItemData(tiSelected))->GetBaseAdress();
		wxstrTitle.Printf(L"0x%p", pcvoidRegionStart);
		tiIdIterator = tiSelected;
		break;
	}
	/*if (tiIdIterator == nullptr)
	{
	tiIdIterator = tiSelected;
	wxstrAddress.Printf("0x%p", static_cast<Tree_Item_Region*>(m_ptrVMContentDisplay->m_ptrTreeCtrl->GetItemData(tiIdIterator))->GetBaseAdress());
	}
	else
	{
	wxstrAddress.Append(wxstrAddress.Format("0x%p [+]", static_cast<Tree_Item_Allocation_Base*>(m_ptrVMContentDisplay->m_ptrTreeCtrl->GetItemData(tiSelected))->GetAllocationBase()));
	}*/
	//wxString wxstrFoo = wxString::Printf("%s", L"ký tự");
	m_ptrVMContentDisplay->SetTitle(wxString::Format(wxT("%s từ %s"), m_ptrVMContentDisplay->m_OutputType == OUTPUT_TYPE_ASM ? wxT("Mã hợp ngữ") : wxT("Các dãy ký tự"), wxstrTitle));
	wxstrTitle = m_ptrVMContentDisplay->GetTitle();
	UINT_PTR uintLastExaminedAddress = 0;
	do
	{
		if (static_cast<Generic_Tree_Item*>(m_ptrVMContentDisplay->m_ptrTreeCtrl->GetItemData(tiIdIterator))->GetType() != TREE_ITEM_TYPE::REGION)
			break;
		//if (this->TestDestroy()) return (wxThread::ExitCode)0; // necessary?
		SIZE_T nRegionSize = static_cast<Tree_Item_Region*>(m_ptrVMContentDisplay->m_ptrTreeCtrl->GetItemData(tiIdIterator))->GetRegionSize();
		pcvoidRegionEnd = (PBYTE)(static_cast<Tree_Item_Region*>(m_ptrVMContentDisplay->m_ptrTreeCtrl->GetItemData(tiIdIterator))->GetBaseAdress()) + nRegionSize - 1;
		SIZE_T nBufferSize = 1024 * 512; // 512KB
		SIZE_T nReadSize = nRegionSize;
		if (nRegionSize > nBufferSize)
		{
			// Don't allocate a huge buffer though.
			if (nRegionSize < 1024 * 1024) // 1MB
			{
				nBufferSize = nRegionSize;
			}
			else
			{
				//wxLogDebug(wxT("RegionSize >= 1MB"));
				nReadSize = nBufferSize;
			}
		}
		LPCVOID BaseAddress = static_cast<Tree_Item_Region*>(m_ptrVMContentDisplay->m_ptrTreeCtrl->GetItemData(tiIdIterator))->GetBaseAdress();
		for (size_t offset = 0; offset <= nRegionSize; offset += nReadSize)
		{
			if (this->TestDestroy()) 
				return (wxThread::ExitCode)0;

			std::unique_ptr<BYTE>smptrAutoFreedByteBuffer(new BYTE[nBufferSize]);
			PBYTE ptrByteBuffer = smptrAutoFreedByteBuffer.get();
			SIZE_T nNumOfBytesRead;
			ReadProcessMemory(m_hProcessToRead, (PBYTE)BaseAddress + offset, ptrByteBuffer, nReadSize, &nNumOfBytesRead);
			switch (m_ptrVMContentDisplay->m_OutputType)
			{
			case OUTPUT_TYPE_ASM:
			{
				csh handle;
				cs_insn *insn;
				size_t count;
				wxString strAssembly;
#ifdef _WIN64
				m_CSMode = FMIP::IsProcessWoW64(m_hProcessToRead) ? CS_MODE_32 : CS_MODE_64;
#else
				m_CSMode = CS_MODE_32;
#endif // _WIN64
				if (cs_open(CS_ARCH_X86, m_CSMode, &handle) != CS_ERR_OK)
				{
					wxLogDebug(wxT("ERROR 1: Failed to disassemble given code!"));
					continue;
				}
				count = cs_disasm(handle, ptrByteBuffer, nReadSize, (UINT_PTR)BaseAddress, 0, &insn);
				if (count > 0)
				{
					for (size_t line = 0; line < count; line++)
					{
						if (this->TestDestroy())
						{
							cs_free(insn, count);
							cs_close(&handle);
							return (wxThread::ExitCode)0; //IMPORTANT TO KILL THE THREAD DURING A LONG DURATION OPERATION.
						}
						strAssembly.append(wxString::Format(wxT("0x%p: %-12s\t%-s\r\n"), (void*)insn[line].address, insn[line].mnemonic,
							insn[line].op_str));
					}
					uintLastExaminedAddress = insn[count - 1].address;
					m_ptrVMContentDisplay->AppendOutput(strAssembly);
					cs_free(insn, count);
				}
				else
				{
					wxLogDebug(wxT("ERROR 2: Failed to disassemble given code!"));
				}
				cs_close(&handle);
			}
			break;

			case OUTPUT_TYPE_STRING:
			{
#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif
#define PAGE_SIZE 0x1000
				ULONG minimumLength;
				SIZE_T displayBufferCount;
				wxString wxstrStrings;
				minimumLength = 4;
				if (minimumLength < 4)
					return (wxThread::ExitCode)1;
				displayBufferCount = (PAGE_SIZE * 2) - 1;
				// no need for std::wstring because it costs more resources and we don't have to do much text manipulation.
				std::unique_ptr<WCHAR>smptrAutoFreedWszString(new WCHAR[displayBufferCount + 1]);
				PWCHAR wzsDisplayBuffer = smptrAutoFreedWszString.get(); 
				//
				UCHAR byte; // current byte
				UCHAR byte1; // previous byte
				UCHAR byte2; // byte before previous byte
				BOOLEAN printable;
				BOOLEAN printable1;
				BOOLEAN printable2;
				ULONG length;
				byte1 = 0;
				byte2 = 0;
				printable1 = FALSE;
				printable2 = FALSE;
				length = 0;
				size_t line = 0;
				for (SIZE_T i = 0; i < nReadSize; i++)
				{
					if (this->TestDestroy()) return (wxThread::ExitCode)0;
					byte = ptrByteBuffer[i];
					printable = PhCharIsPrintable[byte];

					// To find strings Process Hacker uses a state table.
					// * byte2 - byte before previous byte
					// * byte1 - previous byte
					// * byte - current byte
					// * length - length of current string run
					//
					// The states are described below.
					//
					//    [byte2] [byte1] [byte] ...
					//    [char] means printable, [oth] means non-printable.
					//
					// 1. [char] [char] [char] ...
					//      (we're in a non-wide sequence)
					//      -> append char.
					// 2. [char] [char] [oth] ...
					//      (we reached the end of a non-wide sequence, or we need to start a wide sequence)
					//      -> if current string is big enough, create result (non-wide).
					//         otherwise if byte = null, reset to new string with byte1 as first character.
					//         otherwise if byte != null, reset to new string.
					// 3. [char] [oth] [char] ...
					//      (we're in a wide sequence)
					//      -> (byte1 should = null) append char.
					// 4. [char] [oth] [oth] ...
					//      (we reached the end of a wide sequence)
					//      -> (byte1 should = null) if the current string is big enough, create result (wide).
					//         otherwise, reset to new string.
					// 5. [oth] [char] [char] ...
					//      (we reached the end of a wide sequence, or we need to start a non-wide sequence)
					//      -> (excluding byte1) if the current string is big enough, create result (wide).
					//         otherwise, reset to new string with byte1 as first character and byte as
					//         second character.
					// 6. [oth] [char] [oth] ...
					//      (we're in a wide sequence)
					//      -> (byte2 and byte should = null) do nothing.
					// 7. [oth] [oth] [char] ...
					//      (we're starting a sequence, but we don't know if it's a wide or non-wide sequence)
					//      -> append char.
					// 8. [oth] [oth] [oth] ...
					//      (nothing)
					//      -> do nothing.

					if (printable2 && printable1 && printable)
					{
						if (length < displayBufferCount)
						{
							wzsDisplayBuffer[length] = byte;
						}
						length++;
					}
					else if (printable2 && printable1 && !printable)
					{
						if (length >= minimumLength)
						{
							goto CreateResult;
						}
						else if (byte == 0)
						{
							length = 1;
							wzsDisplayBuffer[0] = byte1;
						}
						else
						{
							length = 0;
						}
					}
					else if (printable2 && !printable1 && printable)
					{
						if (byte1 == 0)
						{
							if (length < displayBufferCount)
							{
								wzsDisplayBuffer[length] = byte;
							}
							length++;
						}
					}
					else if (printable2 && !printable1 && !printable)
					{
						if (length >= minimumLength)
						{
							goto CreateResult;
						}
						else
						{
							length = 0;
						}
					}
					else if (!printable2 && printable1 && printable)
					{
						if (length >= minimumLength + 1) // length - 1 >= minimumLength but avoiding underflow
						{
							length--; // exclude byte1
							goto CreateResult;
						}
						else
						{
							length = 2;
							wzsDisplayBuffer[0] = byte1;
							wzsDisplayBuffer[1] = byte;
						}
					}
					else if (!printable2 && printable1 && !printable)
					{
						// Nothing
					}
					else if (!printable2 && !printable1 && printable)
					{
						if (length < displayBufferCount)
						{
							wzsDisplayBuffer[length] = byte;
						}
						length++;
					}
					else if (!printable2 && !printable1 && !printable)
					{
						// Nothing
					}

					goto AfterCreateResult;

				CreateResult:
					{
						ULONG lengthInBytes;
						ULONG bias;
						BOOLEAN isWide;
						ULONG displayLength;
						lengthInBytes = length;
						bias = 0;
						isWide = FALSE;
						if (printable1 == printable) // determine if string was wide (refer to state table, 4 and 5)
						{
							isWide = TRUE;
							lengthInBytes *= 2;
						}

						if (printable) // byte1 excluded (refer to state table, 5)
						{
							bias = 1;
						}
						uintLastExaminedAddress = (uint64_t)BaseAddress + (i - bias - lengthInBytes);
						displayLength = (ULONG)(min(length, displayBufferCount));
						wzsDisplayBuffer[displayLength] = L'\0';
						// No need for std::regex since we don't have to do much text manipulation. Manipulating C wide chars in this case is more convenient and resource saving.
						for (size_t i = 0; i<displayLength; i++)
						{
							if (wzsDisplayBuffer[i] == L'\r' || wzsDisplayBuffer[i] == '\n')
								wzsDisplayBuffer[i] = L'\x1A';
						}
						wzsDisplayBuffer[displayLength] = L'\r';
						wzsDisplayBuffer[displayLength + 1] = L'\n';
						wzsDisplayBuffer[displayLength + 2] = L'\0';
						wxstrStrings.append(wxString::Format(wxT("0x%p: "), (void*)uintLastExaminedAddress));
						wxstrStrings.append(wzsDisplayBuffer);
						length = 0;
					}
				AfterCreateResult:
					byte2 = byte1;
					byte1 = byte;
					printable2 = printable1;
					printable1 = printable;
				}
				m_ptrVMContentDisplay->AppendOutput(wxstrStrings);
			}
			break;
			default:
				break;
			} // switch (m_OutputType).
		} // for (size_t offset = 0; offset < nRegionSize; offset += nReadSize).
		if (tiIdIterator == tiSelected)
			break; // this is not a Tree_Item_Allocation_Base, so just process this Tree_Item_Region and exit the loop.
	} while ((tiIdIterator = m_ptrVMContentDisplay->m_ptrTreeCtrl->GetNextSibling(tiIdIterator)) != nullptr);
	if (uintLastExaminedAddress == 0)
		m_ptrVMContentDisplay->SetTitle(wxString::Format(wxT("Không có %s từ 0x%p đến 0x%p"), m_ptrVMContentDisplay->m_OutputType == OUTPUT_TYPE_ASM ? wxT("mã hợp ngữ") : wxT("các dãy ký tự"),
			pcvoidRegionStart, pcvoidRegionEnd));
	else
		m_ptrVMContentDisplay->SetTitle(wxstrTitle.Append(wxString::Format(wxT(" đến 0x%p"), pcvoidRegionEnd)));
	m_ptrVMContentDisplay->m_ptrStatusBar->SetStatusText(wxstrProcessNamePId);
	CloseHandle(m_hProcessToRead);
	return (wxThread::ExitCode)0;
}