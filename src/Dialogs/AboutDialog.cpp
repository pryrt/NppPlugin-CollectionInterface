// This file is part of DoxyIt.
// 
// Copyright (C)2013 Justin Dailey <dail8859@yahoo.com>
// 
// DoxyIt is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include <WindowsX.h>
#include "PluginDefinition.h"
#include "resource.h"

#ifdef _WIN64
#define BITNESS TEXT("(64 bit)")
#else
#define BITNESS TEXT("(32 bit)")
#endif

#pragma warning(push)
#pragma warning(disable: 4100)
INT_PTR CALLBACK abtDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_INITDIALOG:
		//ConvertStaticToHyperlink(hwndDlg, IDC_GITHUB);
		//ConvertStaticToHyperlink(hwndDlg, IDC_README);
		//Edit_SetText(GetDlgItem(hwndDlg, IDC_VERSION), TEXT("DoxyIt v") VERSION_TEXT TEXT(" ") VERSION_STAGE TEXT(" ") BITNESS);
		Edit_SetText(GetDlgItem(hwndDlg, IDC_VERSION), TEXT("CollectionInterface v???"));

		if (1) {
			// Find Center and then position the window:

			// find App center
			RECT rc;
			HWND hParent = GetParent(hwndDlg);
			::GetClientRect(hParent, &rc);
			POINT center;
			int w = rc.right - rc.left;
			int h = rc.bottom - rc.top;
			center.x = rc.left + w / 2;
			center.y = rc.top + h / 2;
			::ClientToScreen(hParent, &center);

			// and position dialog
			RECT dlgRect;
			::GetClientRect(hwndDlg, &dlgRect);
			int x = center.x - (dlgRect.right - dlgRect.left) / 2;
			int y = center.y - (dlgRect.bottom - dlgRect.top) / 2;
			::SetWindowPos(hwndDlg, HWND_TOP, x, y, (dlgRect.right - dlgRect.left), (dlgRect.bottom - dlgRect.top), SWP_SHOWWINDOW);
		}

		return true;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			EndDialog(hwndDlg, 0);
			DestroyWindow(hwndDlg);
			return true;
			//case IDC_GITHUB:
			//	ShellExecute(hwndDlg, TEXT("open"), TEXT("https://github.com/dail8859/DoxyIt/"), NULL, NULL, SW_SHOWNORMAL);
			//	return true;
			//case IDC_README:
			//	ShellExecute(hwndDlg, TEXT("open"), TEXT("https://github.com/dail8859/DoxyIt/blob/v") VERSION_TEXT TEXT("/README.md"), NULL, NULL, SW_SHOWNORMAL);
			//	return true;
		}
	case WM_DESTROY:
		DestroyWindow(hwndDlg);
		return true;
	}
	return false;
}
#pragma warning(pop)
