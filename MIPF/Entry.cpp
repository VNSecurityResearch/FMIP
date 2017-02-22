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
 This file starts this program by calling the entry point (function OnInit) in class MIPF.
*/

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include	 <wx/wx.h>
#endif
#include "MIPF.h"
//#include <vld.h> // it seems buggy after the Windows 10 Anniversary Update

//#ifdef __WXMSW__
//#include <wx/msw/msvcrt.h>      // redefines the new() operator 
//#endif

wxIMPLEMENT_APP(MIPF);