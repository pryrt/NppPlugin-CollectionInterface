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
#include "CollectionInterfaceDialog.h"
#include "CollectionInterfaceClass.h"
#include <string>
#include <vector>

CollectionInterface* pobjCI;
void _populate_file_cbx(HWND hwndDlg, std::vector<std::wstring>& vwsList);

#pragma warning(push)
#pragma warning(disable: 4100)
INT_PTR CALLBACK ciDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_INITDIALOG:
	{
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

		// populate Category list
		::SendDlgItemMessage(hwndDlg, IDC_CI_COMBO_CATEGORY, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"UDL"));
		::SendDlgItemMessage(hwndDlg, IDC_CI_COMBO_CATEGORY, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"AutoCompletion"));
		::SendDlgItemMessage(hwndDlg, IDC_CI_COMBO_CATEGORY, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"FunctionList"));
		::SendDlgItemMessage(hwndDlg, IDC_CI_COMBO_CATEGORY, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Theme"));
		const int index2Begin = 0;	// start with UDL selected
		::SendDlgItemMessage(hwndDlg, IDC_CI_COMBO_CATEGORY, CB_SETCURSEL, index2Begin, 0);

		pobjCI = new CollectionInterface;
	}

	return true;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDCANCEL:
		case IDOK:
		case IDC_CI_BTN_DOWNLOAD:
		case IDC_CI_BTN_DONE:
		case IDC_CI_BTN_RESTART:
			EndDialog(hwndDlg, 0);
			DestroyWindow(hwndDlg);
			delete pobjCI;
			pobjCI = NULL;
			return true;
		case IDC_CI_COMBO_CATEGORY:
			if(HIWORD(wParam) == CBN_SELCHANGE) {
				LRESULT selectedIndex = ::SendDlgItemMessage(hwndDlg, IDC_CI_COMBO_CATEGORY, CB_GETCURSEL, 0, 0);
				if (selectedIndex != CB_ERR) {
					LRESULT needLen = ::SendDlgItemMessage(hwndDlg, IDC_CI_COMBO_CATEGORY, CB_GETLBTEXTLEN, selectedIndex, 0);
					std::wstring wsCategory(needLen,0);
					::SendDlgItemMessage(hwndDlg, IDC_CI_COMBO_CATEGORY, CB_GETLBTEXT, selectedIndex, reinterpret_cast<LPARAM>(wsCategory.c_str()));
					if (wsCategory == L"UDL")
						_populate_file_cbx(hwndDlg, pobjCI->vwsUDLFiles);
					else if (wsCategory == L"AutoCompletion")
						_populate_file_cbx(hwndDlg, pobjCI->vwsACFiles);
					else if (wsCategory == L"FunctionList")
						_populate_file_cbx(hwndDlg, pobjCI->vwsFLFiles);
					else if (wsCategory == L"Theme")
						_populate_file_cbx(hwndDlg, pobjCI->vwsThemeFiles);
				}
			}
			return true;
		case IDC_CI_COMBO_FILE:
			if (HIWORD(wParam) == CBN_SELCHANGE) {
				LRESULT selectedIndex = ::SendMessage(reinterpret_cast<HWND>(lParam), CB_GETCURSEL, 0, 0);
				if (selectedIndex != CB_ERR) {
					wchar_t buffer[256];
					LRESULT needLen = ::SendMessage(reinterpret_cast<HWND>(lParam), CB_GETLBTEXTLEN, selectedIndex, 0);
					if (needLen < 256) {
						::SendMessage(reinterpret_cast<HWND>(lParam), CB_GETLBTEXT, selectedIndex, reinterpret_cast<LPARAM>(buffer));
						buffer[255] = '\0';
						::MessageBox(NULL, buffer, L"Which File:", MB_OK);
					}
				}
			}
			return true;
		default:
			return false;
		}
	case WM_DESTROY:
		DestroyWindow(hwndDlg);
		return true;
	}
	return false;
}
#pragma warning(pop)

void _populate_file_cbx(HWND hwndDlg, std::vector<std::wstring>& vwsList)
{
	::SendDlgItemMessage(hwndDlg, IDC_CI_COMBO_FILE, CB_RESETCONTENT, 0, 0);
	::SendDlgItemMessage(hwndDlg, IDC_CI_COMBO_FILE, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"<Pick File>"));
	for (auto& str : vwsList) {
		::SendDlgItemMessage(hwndDlg, IDC_CI_COMBO_FILE, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(str.c_str()));
	}
	::SendDlgItemMessage(hwndDlg, IDC_CI_COMBO_FILE, CB_SETCURSEL, 0, 0);
	return;
}
