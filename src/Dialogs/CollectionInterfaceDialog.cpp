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
#include "menuCmdID.h"
#include <string>
#include <vector>

CollectionInterface* pobjCI;
void _populate_file_cbx(HWND hwndDlg, std::vector<std::string>& vsList);
void _populate_file_cbx(HWND hwndDlg, std::map<std::string, std::string>& mapURLs, std::map<std::string, std::string>& mapDisplay);

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
		//pobjCI->getListsFromJson();
		_populate_file_cbx(hwndDlg, pobjCI->mapUDL, pobjCI->mapDISPLAY);
	}

	return true;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDCANCEL:
		case IDOK:
		case IDC_CI_BTN_DONE:
			EndDialog(hwndDlg, 0);
			DestroyWindow(hwndDlg);
			delete pobjCI;
			pobjCI = NULL;
			return true;
		case IDC_CI_COMBO_CATEGORY:
			if (HIWORD(wParam) == CBN_SELCHANGE) {
				LRESULT selectedIndex = ::SendDlgItemMessage(hwndDlg, IDC_CI_COMBO_CATEGORY, CB_GETCURSEL, 0, 0);
				if (selectedIndex != CB_ERR) {
					LRESULT needLen = ::SendDlgItemMessage(hwndDlg, IDC_CI_COMBO_CATEGORY, CB_GETLBTEXTLEN, selectedIndex, 0);
					std::wstring wsCategory(needLen, 0);
					::SendDlgItemMessage(hwndDlg, IDC_CI_COMBO_CATEGORY, CB_GETLBTEXT, selectedIndex, reinterpret_cast<LPARAM>(wsCategory.c_str()));
					if (wsCategory == L"UDL")
						_populate_file_cbx(hwndDlg, pobjCI->mapUDL, pobjCI->mapDISPLAY);
					else if (wsCategory == L"AutoCompletion")
						_populate_file_cbx(hwndDlg, pobjCI->mapAC, pobjCI->mapDISPLAY);
					else if (wsCategory == L"FunctionList")
						_populate_file_cbx(hwndDlg, pobjCI->mapFL, pobjCI->mapDISPLAY);
					else if (wsCategory == L"Theme")
						_populate_file_cbx(hwndDlg, pobjCI->vThemeFiles);
				}
			}
			return true;
		case IDC_CI_COMBO_FILE:
			if (HIWORD(wParam) == CBN_SELCHANGE) {
				LRESULT selectedIndex = ::SendMessage(reinterpret_cast<HWND>(lParam), CB_GETCURSEL, 0, 0);
				if (!selectedIndex) {
					::MessageBox(NULL, L"Please pick a FILE, not <Pick File>", L"FILE selector", MB_ICONWARNING);
				} 
				else if (selectedIndex != CB_ERR) {
					LRESULT needLen = ::SendMessage(reinterpret_cast<HWND>(lParam), CB_GETLBTEXTLEN, selectedIndex, 0);
					std::wstring wsFilename(needLen, 0);
					::SendMessage(reinterpret_cast<HWND>(lParam), CB_GETLBTEXT, selectedIndex, reinterpret_cast<LPARAM>(wsFilename.c_str()));
					//::MessageBox(NULL, wsFilename.c_str(), L"Which File:", MB_OK);
				}
			}
			return true;
		case IDC_CI_BTN_RESTART:
		{
			// In python
			//		#console.write(f"argv => {sys.argv}\n") # this would be useful for the['cmd', 'o1', ...'oN'] version of Popen = > subprocess.Popen(sys.argv)
			//		#   but since I want the TIMEOUT to give previous instance a chance to close, I need to use the string from GetCommandLine() anyway,
			//		#   so don't need sys.argv
			//
			//		cmd = f"cmd /C TIMEOUT /T 2 && {GetCommandLine()}"
			//		subprocess.Popen(cmd)
			//		self.terminate()
			//		notepad.menuCommand(MENUCOMMAND.FILE_EXIT)
			STARTUPINFOW si;
			PROCESS_INFORMATION pi;

			ZeroMemory(&si, sizeof(si));
			si.cb = sizeof(si);
			ZeroMemory(&pi, sizeof(pi));

			wchar_t cmd[MAX_PATH];
			swprintf_s(cmd, L"cmd /C ECHO Wait for old Notepad++ to close before launching new copy... & TIMEOUT /T 6 && START \"\" %s", GetCommandLineW());
			//::MessageBox(NULL, cmd, L"Command to launch:", MB_OK);
			// If I want to create a persistent console (`cmd /K`), use CREATE_NEW_CONSOLE
			// if it doesn't need a persistent console (`cmd /C`), use DETACHED_PROCESS
			//	either one will outlive the parent, so File>Exit will work
			//	!!TODO!! get the delay working right, or punt to creating and launching a temporary .bat that deletes itself
			if (CreateProcessW(nullptr, cmd, nullptr, nullptr, FALSE, CREATE_NEW_CONSOLE, nullptr, nullptr, &si, &pi)) {
				CloseHandle(pi.hProcess);
				CloseHandle(pi.hThread);

				HWND hParent = GetParent(hwndDlg);
				SendMessage(hParent, NPPM_MENUCOMMAND, 0, IDM_FILE_EXIT);
			}
			else {
				DWORD errNum = GetLastError();
				LPWSTR messageBuffer = nullptr;
				size_t size = FormatMessageW(
					FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
					NULL,
					errNum,
					MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
					(LPWSTR)&messageBuffer,
					0,
					NULL
				);

				wchar_t msg[4096];
				swprintf_s(msg, L"Command: %s\n\nCode:    %d\nDesc:    [%lld]%s", cmd, errNum, size, messageBuffer);
				::MessageBox(NULL, msg, L"Command Error", MB_ICONERROR);

				LocalFree(messageBuffer);
			}
		}
		return true;
		case IDC_CI_BTN_DOWNLOAD:
		{
			LRESULT selectedCatIndex = ::SendDlgItemMessage(hwndDlg, IDC_CI_COMBO_CATEGORY, CB_GETCURSEL, 0, 0);
			if (selectedCatIndex == CB_ERR) {
				::MessageBox(NULL, L"Could not understand CATEGORY; sorry", L"Download Error", MB_ICONERROR);
				return true;
			}

			// use the ASCII version, because the mapping is in std::string, not std::wstring, to avoid multiple conversions
			LRESULT needCatLen = ::SendDlgItemMessageA(hwndDlg, IDC_CI_COMBO_CATEGORY, CB_GETLBTEXTLEN, selectedCatIndex, 0);
			std::string sCategory(needCatLen, 0);
			::SendDlgItemMessageA(hwndDlg, IDC_CI_COMBO_CATEGORY, CB_GETLBTEXT, selectedCatIndex, reinterpret_cast<LPARAM>(sCategory.c_str()));

			LRESULT selectedFileIndex = ::SendDlgItemMessage(hwndDlg, IDC_CI_COMBO_FILE, CB_GETCURSEL, 0, 0);
			switch (selectedFileIndex) {
			case CB_ERR:
				::MessageBox(NULL, L"Could not understand FILE combobox; sorry", L"Download Error", MB_ICONERROR);
				return true;
			case 0:
				::MessageBox(NULL, L"Please pick a FILE, not <Pick File>", L"Download Error", MB_ICONERROR);
				return true;
			}

			LRESULT needFileLen = ::SendDlgItemMessageA(hwndDlg, IDC_CI_COMBO_FILE, CB_GETLBTEXTLEN, selectedFileIndex, 0);
			std::string sFilename(needFileLen, 0);
			::SendDlgItemMessageA(hwndDlg, IDC_CI_COMBO_FILE, CB_GETLBTEXT, selectedFileIndex, reinterpret_cast<LPARAM>(sFilename.c_str()));

			std::string id_name = (pobjCI->revDISPLAY.count(sFilename)) ? pobjCI->revDISPLAY[sFilename] : "!!DoesNotExist!!";
			std::string sURL = "";
			if (sCategory == "UDL") {
				if(pobjCI->mapUDL.count(id_name)) {
					sURL = pobjCI->mapUDL[id_name];
				}
			}
			else if (sCategory == "AutoCompletion") {
				if (pobjCI->mapAC.count(id_name)) {
					sURL = pobjCI->mapAC[id_name];
				}
			}
			else if (sCategory == "FunctionList") {
				if (pobjCI->mapFL.count(id_name)) {
					sURL = pobjCI->mapFL[id_name];
				}
			}
			else if (sCategory == "Theme") {
				sURL = "https://raw.githubusercontent.com/notepad-plus-plus/nppThemes/main/themes/" + sFilename;
			}

			std::string sMessage = "TODO: Download(" + sCategory + ", " + sFilename + ") => " + sURL;
			::MessageBoxA(NULL, sMessage.c_str(), "TODO: Download", MB_OK);
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

void _populate_file_cbx(HWND hwndDlg, std::vector<std::string>& vsList)
{
	::SendDlgItemMessage(hwndDlg, IDC_CI_COMBO_FILE, CB_RESETCONTENT, 0, 0);
	::SendDlgItemMessage(hwndDlg, IDC_CI_COMBO_FILE, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"<Pick File>"));
	for (auto& str : vsList) {
		::SendDlgItemMessageA(hwndDlg, IDC_CI_COMBO_FILE, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(str.c_str()));
	}
	::SendDlgItemMessage(hwndDlg, IDC_CI_COMBO_FILE, CB_SETCURSEL, 0, 0);
	return;
}

void _populate_file_cbx(HWND hwndDlg, std::map<std::string,std::string>& mapURLs, std::map<std::string,std::string>& mapDisplay)
{
	::SendDlgItemMessage(hwndDlg, IDC_CI_COMBO_FILE, CB_RESETCONTENT, 0, 0);
	::SendDlgItemMessage(hwndDlg, IDC_CI_COMBO_FILE, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"<Pick File>"));
	for (const auto& pair: mapURLs) {
		auto key = pair.first;
		if (mapDisplay.count(key)) {
			::SendDlgItemMessageA(hwndDlg, IDC_CI_COMBO_FILE, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(mapDisplay[key].c_str()));
		}
	}
	::SendDlgItemMessage(hwndDlg, IDC_CI_COMBO_FILE, CB_SETCURSEL, 0, 0);
	return;
}
