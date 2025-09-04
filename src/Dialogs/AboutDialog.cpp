/*
	Copyright (C) 2025  Peter C. Jones <pryrtcode@pryrt.com>

	This file is part of the source code for the CollectionInterface plugin for Notepad++

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <WindowsX.h>
#include "PluginDefinition.h"
#include "resource.h"
#include "Version.h"
#include "Hyperlinks.h"
#include "NppMetaClass.h"

#ifdef _WIN64
#define BITNESS TEXT("(64 bit)")
#else
#define BITNESS TEXT("(32 bit)")
#endif

HWND g_hwndAboutDlg = nullptr;

#pragma warning(push)
#pragma warning(disable: 4100)
INT_PTR CALLBACK abtDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
		case WM_INITDIALOG: {
			// need to know versions for making darkMode decisions
			gNppMetaInfo.populate();
			LRESULT darkdialogVersion = gNppMetaInfo.verDotToLL(8, 5, 4, 0);	// requires 8.5.4.0 for NPPM_DARKMODESUBCLASSANDTHEME (NPPM_GETDARKMODECOLORS was 8.4.1, so covered)

			// prepare to follow DarkMode
			g_hwndAboutDlg = hwndDlg;
			bool isDM = (bool)::SendMessage(nppData._nppHandle, NPPM_ISDARKMODEENABLED, 0, 0);
			if (isDM && gNppMetaInfo.isNppVerAtLeast(darkdialogVersion)) {
				NppDarkMode::Colors myColors = { 0 };
				if (::SendMessage(nppData._nppHandle, NPPM_GETDARKMODECOLORS, sizeof(NppDarkMode::Colors), reinterpret_cast<LPARAM>(&myColors))) {
					SetHyperlinkRGB(myColors.linkText);
				}
				else {
					SetHyperlinkRGB(RGB(64, 64, 255));
				}
			}
			else {
				SetHyperlinkRGB(RGB(0, 0, 192));
			}
			//ConvertStaticToHyperlink(hwndDlg, IDC_README);
			//Edit_SetText(GetDlgItem(hwndDlg, IDC_VERSION), TEXT("DoxyIt v") VERSION_TEXT TEXT(" ") VERSION_STAGE TEXT(" ") BITNESS);
			wchar_t title[256];
			swprintf_s(title, L"%s v%s%s %s", VERSION_NAME_WS, VERSION_TEXT, VERSION_STAGE_WS, BITNESS);
			Edit_SetText(GetDlgItem(hwndDlg, IDC_VERSION), title);

			// subclass-and-theme should come "after" all controls set up...
			if (isDM && gNppMetaInfo.isNppVerAtLeast(darkdialogVersion))
				::SendMessage(nppData._nppHandle, NPPM_DARKMODESUBCLASSANDTHEME, static_cast<WPARAM>(NppDarkMode::dmfInit), reinterpret_cast<LPARAM>(g_hwndAboutDlg));

			// however, it overwrites the ConverStaticToHyperlink, so I need to go back and do that _after_ the message
			ConvertStaticToHyperlink(hwndDlg, IDC_GITHUB);

			//// Finally, figure out where to draw the dialog, and draw it.
			RECT rc;
			HWND hParent = GetParent(hwndDlg);
			::GetClientRect(hParent, &rc);
			POINT center;
			int w = rc.right - rc.left;
			int h = rc.bottom - rc.top;
			center.x = rc.left + w / 2;
			center.y = rc.top + h / 2;
			::ClientToScreen(hParent, &center);
			RECT dlgRect;
			::GetClientRect(hwndDlg, &dlgRect);
			int x = center.x - (dlgRect.right - dlgRect.left) / 2;
			int y = center.y - (dlgRect.bottom - dlgRect.top) / 2;
			::SetWindowPos(hwndDlg, HWND_TOP, x, y, (dlgRect.right - dlgRect.left), (dlgRect.bottom - dlgRect.top), SWP_SHOWWINDOW);

			return true;
		}
		case WM_COMMAND:
		{
			switch (LOWORD(wParam)) {
				case IDCANCEL:
				case IDOK:
					EndDialog(hwndDlg, 0);
					DestroyWindow(hwndDlg);
					g_hwndAboutDlg = nullptr;
					return true;
				case IDC_GITHUB:
					ShellExecute(hwndDlg, TEXT("open"), TEXT(VERSION_URL), NULL, NULL, SW_SHOWNORMAL);
					return true;
					//case IDC_README:
					//	ShellExecute(hwndDlg, TEXT("open"), TEXT("https://github.com/dail8859/DoxyIt/blob/v") VERSION_TEXT TEXT("/README.md"), NULL, NULL, SW_SHOWNORMAL);
					//	return true;
			}
		}
		case WM_DESTROY:
		{
			g_hwndAboutDlg = nullptr;
			DestroyWindow(hwndDlg);
			return true;
		}
	}
	return false;
}
#pragma warning(pop)
