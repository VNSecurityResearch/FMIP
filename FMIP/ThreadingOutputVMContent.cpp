﻿#include "wx/wxprec.h"
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include "FMIP.h"
#include "ThreadingOutputVMContent.h"
#include <memory>
//#include "MemLeakDetection.h"
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
	} while (TreeItemType != TREE_ITEM_TYPE_PROCESS_NAME_PID);
	wxstrProcessNamePId = m_ptrVMContentDisplay->m_ptrTreeCtrl->GetItemText(tiIdIterator);
	//m_ptrVMContentDisplay->SetStatusText(strProcessNamePId);
	m_ptrId = static_cast<Tree_Item_ptrrocess_Name_PId*>(ptrGenericTreeItem)->GetPId();
	m_hProcessToRead = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, m_ptrId);
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
	void* ptrRegionStart = nullptr;
	void* ptrRegionEnd = nullptr;
	switch (TreeItemType)
	{
	case TREE_ITEM_TYPE_ALLOCATION_BASE:
		ptrRegionStart = static_cast<Tree_Item_Allocation_Base*>(m_ptrVMContentDisplay->m_ptrTreeCtrl->GetItemData(tiSelected))->GetAllocationBase();
		wxstrTitle.Append(wxstrTitle.Format("0x%p [+]", ptrRegionStart));
		tiIdIterator = m_ptrVMContentDisplay->m_ptrTreeCtrl->GetFirstChild(tiSelected, tiIdValue);
		break;
	case TREE_ITEM_TYPE_REGION:
		ptrRegionStart = static_cast<Tree_Item_Region*>(m_ptrVMContentDisplay->m_ptrTreeCtrl->GetItemData(tiSelected))->GetBaseAdress();
		wxstrTitle.Printf("0x%p", ptrRegionStart);
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
	uint64_t uintLastExaminedAddress = 0;
	do
	{
		if (static_cast<Generic_Tree_Item*>(m_ptrVMContentDisplay->m_ptrTreeCtrl->GetItemData(tiIdIterator))->GetType() != TREE_ITEM_TYPE::TREE_ITEM_TYPE_REGION)
			break;
		//if (this->TestDestroy()) return (wxThread::ExitCode)0; // necessary?
		SIZE_T nRegionSize = static_cast<Tree_Item_Region*>(m_ptrVMContentDisplay->m_ptrTreeCtrl->GetItemData(tiIdIterator))->GetRegionSize();
		ptrRegionEnd = (PBYTE)(static_cast<Tree_Item_Region*>(m_ptrVMContentDisplay->m_ptrTreeCtrl->GetItemData(tiIdIterator))->GetBaseAdress()) + nRegionSize - 1;
		SIZE_T nBufferSize = 4096 * 16;
		SIZE_T nReadSize = nRegionSize;
		if (nRegionSize > nBufferSize)
		{
			// Don't allocate a huge buffer though.
			if (nRegionSize < 16 * 1024 * 1024) // 16MB
			{
				nBufferSize = nRegionSize;
			}
			else
			{
				wxLogDebug(wxT("RegionSize >= 16MB"));
				nReadSize = nBufferSize;
			}
		}
		LPCVOID BaseAddress = static_cast<Tree_Item_Region*>(m_ptrVMContentDisplay->m_ptrTreeCtrl->GetItemData(tiIdIterator))->GetBaseAdress();
		for (size_t offset = 0; offset <= nRegionSize; offset += nReadSize)
		{
			if (this->TestDestroy()) return (wxThread::ExitCode)0;
			std::unique_ptr<unsigned char>sptrCharBuffer(new unsigned char[nBufferSize]);
			SIZE_T nNumOfBytesRead;
			ReadProcessMemory(m_hProcessToRead, (PBYTE)BaseAddress + offset, (sptrCharBuffer.get()), nReadSize, &nNumOfBytesRead);
			switch (m_ptrVMContentDisplay->m_OutputType)
			{
			case OUTPUT_TYPE_ASM:
			{
				csh handle;
				cs_insn *insn;
				size_t count;
				wxString strAssembly;
#ifdef _WIN64
				m_CSMode = FMIP_::IsProcessWoW64(m_hProcessToRead) ? CS_MODE_32 : CS_MODE_64;
#else
				m_CSMode = CS_MODE_32;
#endif // _WIN64
				if (cs_open(CS_ARCH_X86, m_CSMode, &handle) != CS_ERR_OK)
				{
					wxLogDebug(wxT("ERROR 1: Failed to disassemble given code!"));
					continue;
				}
				count = cs_disasm(handle, sptrCharBuffer.get(), nReadSize, (uint64_t)BaseAddress, 0, &insn);
				if (count > 0)
				{
					for (size_t line = 0; line < count; line++)
					{
						if (this->TestDestroy()) return (wxThread::ExitCode)0; //IMPORTANT TO KILL THE THREAD WHILE IN LONG DURATION OPERATION
						strAssembly.append(wxString::Format("0x%p: %-12s\t%-s\r\n", (void*)insn[line].address, insn[line].mnemonic,
							insn[line].op_str));
					}
					uintLastExaminedAddress = insn[count - 1].address;
					m_ptrVMContentDisplay->Output(strAssembly);
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
				BOOLEAN detectUnicode;
				ULONG memoryTypeMask;
				PUCHAR buffer;
				SIZE_T bufferSize;
				SIZE_T displayBufferCount;
				wxString wxstrStrings;
				minimumLength = 4;
				if (minimumLength < 4)
					return "";
				displayBufferCount = (PAGE_SIZE * 2) - 1;
				std::wstring wstrDisplayBuffer(displayBufferCount + 1, L'\0');
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
					byte = (sptrCharBuffer.get())[i];
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
							wstrDisplayBuffer.at(length) = byte;
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
							wstrDisplayBuffer.at(0) = byte1;
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
								wstrDisplayBuffer.at(length) = byte;
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
							wstrDisplayBuffer.at(0) = byte1;
							wstrDisplayBuffer.at(1) = byte;
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
							wstrDisplayBuffer.at(length) = byte;
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
						wstrDisplayBuffer.at(displayLength) = L'\n';
						//displayBuffer.get()[displayLength + 1] = L'\n';
						wstrDisplayBuffer.at(displayLength + 1) = L'\0';
						int pos = 0;
						pos = wstrDisplayBuffer.find(L'\r');
						while (pos != std::wstring::npos)
						{
							wstrDisplayBuffer.replace(pos, 1, L"\x1A\x0"); // L"\r                    ");
							pos = wstrDisplayBuffer.find(L'\r', pos + 1); // should be pos + 21?
						}
						pos = wstrDisplayBuffer.find(L'\n');
						int intStringLength = wcslen(wstrDisplayBuffer.data());
						while (pos != std::wstring::npos && pos < intStringLength - 1)
						{
							wstrDisplayBuffer.replace(pos, 1, L"\x1a\x0"); // L"\r                    ");
							pos = wstrDisplayBuffer.find(L'\n', pos + 1);
						}
						wxstrStrings.append(wxString::Format("0x%p: ", (void*)uintLastExaminedAddress));
						wxstrStrings.append(wstrDisplayBuffer.data());
						length = 0;
					}
				AfterCreateResult:
					byte2 = byte1;
					byte1 = byte;
					printable2 = printable1;
					printable1 = printable;
				}
				m_ptrVMContentDisplay->Output(wxstrStrings);
			}
			break;
			default:
				break;
			} // switch (m_OutputType)
		} // for (size_t offset = 0; offset < nRegionSize; offset += nReadSize)
		if (tiIdIterator == tiSelected)
			break; // this is not a Tree_Item_Allocation_Base, so just process this Tree_Item_Region and exit the loop
	} while ((tiIdIterator = m_ptrVMContentDisplay->m_ptrTreeCtrl->GetNextSibling(tiIdIterator)) != nullptr);
	if (uintLastExaminedAddress == 0)
		m_ptrVMContentDisplay->SetTitle(wxString::Format(wxT("Không có %s từ 0x%p đến 0x%p"), m_ptrVMContentDisplay->m_OutputType == OUTPUT_TYPE_ASM ? wxT("mã hợp ngữ") : wxT("các dãy ký tự"),
			ptrRegionStart, ptrRegionEnd));
	else
		m_ptrVMContentDisplay->SetTitle(wxstrTitle.Append(wxString::Format(wxT(" đến 0x%p"), (void*)uintLastExaminedAddress)));
	m_ptrVMContentDisplay->m_ptrStatusBar->SetStatusText(wxstrProcessNamePId);
	CloseHandle(m_hProcessToRead);
	return (wxThread::ExitCode)0;
}