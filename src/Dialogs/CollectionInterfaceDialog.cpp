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
#include <CommCtrl.h>

CollectionInterface* pobjCI;
void _populate_file_cbx(HWND hwndDlg, std::vector<std::wstring>& vsList);
void _populate_file_cbx(HWND hwndDlg, std::map<std::wstring, std::wstring>& mapURLs, std::map<std::wstring, std::wstring>& mapDisplay);
std::wstring _get_tab_category_wstr(HWND hwndDlg, int idcTabCtrl);

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

		// populate Category list into the tab bar: "INSERTITEM" means RightToLeft, because it inserts it before the earlier tab.
		std::wstring ws;
		TCITEM pop;
		pop.mask = TCIF_TEXT;
		ws = L"Theme"; pop.cchTextMax = static_cast<int>(ws.size()); pop.pszText = const_cast<LPWSTR>(ws.data());
		::SendDlgItemMessage(hwndDlg, IDC_CI_TABCTRL, TCM_INSERTITEM, 0, reinterpret_cast<LPARAM>(&pop));
		ws = L"FunctionList"; pop.cchTextMax = static_cast<int>(ws.size()); pop.pszText = const_cast<LPWSTR>(ws.data());
		::SendDlgItemMessage(hwndDlg, IDC_CI_TABCTRL, TCM_INSERTITEM, 0, reinterpret_cast<LPARAM>(&pop));
		ws = L"AutoCompletion"; pop.cchTextMax = static_cast<int>(ws.size()); pop.pszText = const_cast<LPWSTR>(ws.data());
		::SendDlgItemMessage(hwndDlg, IDC_CI_TABCTRL, TCM_INSERTITEM, 0, reinterpret_cast<LPARAM>(&pop));
		ws = L"UDL"; pop.cchTextMax = static_cast<int>(ws.size()); pop.pszText = const_cast<LPWSTR>(ws.data());
		::SendDlgItemMessage(hwndDlg, IDC_CI_TABCTRL, TCM_INSERTITEM, 0, reinterpret_cast<LPARAM>(&pop));
		::SendDlgItemMessage(hwndDlg, IDC_CI_TABCTRL, TCM_SETCURSEL, 0, 0);

		// disable options
		HWND hwCHK = GetDlgItem(hwndDlg, IDC_CI_CHK_ALSO_AC);
		EnableWindow(hwCHK, FALSE);	// ShowWindow(hwCHK, SW_HIDE);
		hwCHK = GetDlgItem(hwndDlg, IDC_CI_CHK_ALSO_FL);
		EnableWindow(hwCHK, FALSE);	// ShowWindow(hwCHK, SW_HIDE);

		// set progress bar
		::SendDlgItemMessage(hwndDlg, IDC_CI_PROGRESSBAR, PBM_SETPOS, 0, 0);

		pobjCI = new CollectionInterface(hParent);
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
		case IDC_CI_COMBO_FILE:
			if (HIWORD(wParam) == LBN_SELCHANGE) {
				LRESULT selectedIndex = ::SendMessage(reinterpret_cast<HWND>(lParam), LB_GETCURSEL, 0, 0);
				if (selectedIndex != LB_ERR) {
					LRESULT needLen = ::SendMessage(reinterpret_cast<HWND>(lParam), LB_GETTEXTLEN, selectedIndex, 0);
					std::wstring wsFilename(needLen, 0);
					::SendMessage(reinterpret_cast<HWND>(lParam), LB_GETTEXT, selectedIndex, reinterpret_cast<LPARAM>(wsFilename.data()));
					//::MessageBox(NULL, wsFilename.c_str(), L"Which File:", MB_OK);

					// look at pobjCI->revDISPLAY[]
					std::wstring ws_id_name = (pobjCI->revDISPLAY.count(wsFilename)) ? pobjCI->revDISPLAY[wsFilename] : L"!!DoesNotExist!!";
					HWND hwCHK = GetDlgItem(hwndDlg, IDC_CI_CHK_ALSO_AC);
					EnableWindow(hwCHK, static_cast<BOOL>(pobjCI->mapAC.count(ws_id_name)));
					hwCHK = GetDlgItem(hwndDlg, IDC_CI_CHK_ALSO_FL);
					EnableWindow(hwCHK, static_cast<BOOL>(pobjCI->mapFL.count(ws_id_name)));

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
				swprintf_s(msg, L"Command: %s\n\nCode:    %d\nDesc:    [%lld]%s", cmd, errNum, static_cast<INT64>(size), messageBuffer);
				::MessageBox(NULL, msg, L"Command Error", MB_ICONERROR);

				LocalFree(messageBuffer);
			}
		}
		return true;
		case IDC_CI_BTN_DOWNLOAD:
		{
			std::wstring wsCategory = _get_tab_category_wstr(hwndDlg, IDC_CI_TABCTRL);
			LRESULT selectedFileIndex = ::SendDlgItemMessage(hwndDlg, IDC_CI_COMBO_FILE, LB_GETCURSEL, 0, 0);
			switch (selectedFileIndex) {
			case CB_ERR:
				::MessageBox(NULL, L"Could not understand FILE combobox; sorry", L"Download Error", MB_ICONERROR);
				return true;
			}

			LRESULT needFileLen = ::SendDlgItemMessage(hwndDlg, IDC_CI_COMBO_FILE, LB_GETTEXTLEN, selectedFileIndex, 0);
			std::wstring wsFilename(needFileLen, 0);
			::SendDlgItemMessage(hwndDlg, IDC_CI_COMBO_FILE, LB_GETTEXT, selectedFileIndex, reinterpret_cast<LPARAM>(wsFilename.data()));

			std::wstring ws_id_name = (pobjCI->revDISPLAY.count(wsFilename)) ? pobjCI->revDISPLAY[wsFilename] : L"!!DoesNotExist!!";
			std::wstring wsURL = L"";
			std::wstring wsPath = L"";
			bool isWritable = false;
			std::map<std::wstring, std::map<std::wstring, std::wstring>> alsoDownload;
			std::map<std::wstring, bool> extraWritable;
			if (wsCategory == L"UDL") {
				if (pobjCI->mapUDL.count(ws_id_name)) {
					wsURL = pobjCI->mapUDL[ws_id_name];
				}

				wsPath = pobjCI->nppCfgUdlDir() + L"\\" + ws_id_name + L".xml";
				isWritable = pobjCI->isUdlDirWritable();

				// if chkAC, then also download AC
				bool isCHK = BST_CHECKED == IsDlgButtonChecked(hwndDlg, IDC_CI_CHK_ALSO_AC);
				bool isEN = IsWindowEnabled(GetDlgItem(hwndDlg, IDC_CI_CHK_ALSO_AC));
				if (isEN && isCHK && pobjCI->mapAC.count(ws_id_name)) {
					alsoDownload[L"AC"][L"URL"] = pobjCI->mapAC[ws_id_name];
					alsoDownload[L"AC"][L"PATH"] = pobjCI->nppCfgAutoCompletionDir() + L"\\" + ws_id_name + L".xml";
					extraWritable[L"AC"] = pobjCI->isAutoCompletionDirWritable();
				}

				// if cjkFL, then also download FL
				isCHK = BST_CHECKED == IsDlgButtonChecked(hwndDlg, IDC_CI_CHK_ALSO_FL);
				isEN = IsWindowEnabled(GetDlgItem(hwndDlg, IDC_CI_CHK_ALSO_FL));
				if (isEN && isCHK && pobjCI->mapFL.count(ws_id_name)) {
					alsoDownload[L"FL"][L"URL"] = pobjCI->mapFL[ws_id_name];
					alsoDownload[L"FL"][L"PATH"] = pobjCI->nppCfgFunctionListDir() + L"\\" + ws_id_name + L".xml";
					extraWritable[L"FL"] = pobjCI->isFunctionListDirWritable();
				}
			}
			else if (wsCategory == L"AutoCompletion") {
				if (pobjCI->mapAC.count(ws_id_name)) {
					wsURL = pobjCI->mapAC[ws_id_name];
				}

				wsPath = pobjCI->nppCfgAutoCompletionDir() + L"\\" + ws_id_name + L".xml";
				isWritable = pobjCI->isAutoCompletionDirWritable();
			}
			else if (wsCategory == L"FunctionList") {
				if (pobjCI->mapFL.count(ws_id_name)) {
					wsURL = pobjCI->mapFL[ws_id_name];
				}

				wsPath = pobjCI->nppCfgFunctionListDir() + L"\\" + ws_id_name + L".xml";
				isWritable = pobjCI->isFunctionListDirWritable();
			}
			else if (wsCategory == L"Theme") {
				wsURL = L"https://raw.githubusercontent.com/notepad-plus-plus/nppThemes/main/themes/" + wsFilename;

				wsPath = pobjCI->nppCfgThemesDir() + L"\\" + wsFilename;
				isWritable = pobjCI->isThemesDirWritable();
			}

			if (isWritable) {
				// download directly to the final destination
				pobjCI->downloadFileToDisk(wsURL, wsPath);
				std::wstring msg = L"Downloaded to " + wsPath;
				::MessageBox(hwndDlg, msg.c_str(), L"Download Successful", MB_OK);
			}
			else {
				// download to a temp path, then use ShellExecute(runas) to move it from the temp path to the final destination
				std::wstring wsAsk = L"Cannot write to " + wsPath;
				wsAsk += L"\nI will try again with elevated UAC permission.";
				int ans = ::MessageBox(hwndDlg, wsAsk.c_str(), L"Need Directory Permission", MB_OKCANCEL);
				if (ans == IDOK) {
					std::wstring tmpPath = pobjCI->getWritableTempDir() + L"\\~$TMPFILE.DOWNLOAD.PRYRT.xml";
					pobjCI->downloadFileToDisk(wsURL, tmpPath);
					std::wstring msg = L"Downloaded from\n" + tmpPath + L"\nand moved to\n" + wsPath;
					std::wstring args = L"/C MOVE /Y \"" + tmpPath + L"\" \"" + wsPath + L"\"";
					ShellExecute(hwndDlg, L"runas", L"cmd.exe", args.c_str(), NULL, SW_SHOWMINIMIZED);
					::MessageBox(hwndDlg, msg.c_str(), L"Download and UAC move", MB_OK);
				}
			}

			// also download AC and FL, if applicable
			std::vector<std::wstring> xtra = { L"AC", L"FL" };
			for (auto category : xtra) {
				if (alsoDownload.count(category)) {
					std::wstring xURL = alsoDownload[category][L"URL"];
					std::wstring xPath = alsoDownload[category][L"PATH"];
					if (extraWritable[category]) {
						pobjCI->downloadFileToDisk(xURL, xPath);
					}
					else {
						// download to a temp path, then use ShellExecute(runas) to move it from the temp path to the final destination
						std::wstring wsAsk = L"Cannot write to " + xPath;
						wsAsk += L"\nI will try again with elevated UAC permission.";
						int ans = ::MessageBox(hwndDlg, wsAsk.c_str(), L"Need Directory Permission", MB_OKCANCEL);
						if (ans == IDOK) {
							std::wstring tmpPath = pobjCI->getWritableTempDir() + L"\\~$TMPFILE.DOWNLOAD.PRYRT.xml";
							pobjCI->downloadFileToDisk(xURL, tmpPath);
							std::wstring msg = L"Downloaded from\n" + tmpPath + L"\nand moved to\n" + xPath;
							std::wstring args = L"/C MOVE /Y \"" + tmpPath + L"\" \"" + xPath + L"\"";
							ShellExecute(hwndDlg, L"runas", L"cmd.exe", args.c_str(), NULL, SW_SHOWMINIMIZED);
						}
					}
				}
			}
		}
		return true;
		default:
			return false;
		}
	case WM_NOTIFY:
	{
		if ((wParam == IDC_CI_TABCTRL) && (((LPNMHDR)lParam)->code == TCN_SELCHANGE)) {
			std::wstring wsCategory = _get_tab_category_wstr(hwndDlg, IDC_CI_TABCTRL);
			if (wsCategory == L"UDL") {
				_populate_file_cbx(hwndDlg, pobjCI->mapUDL, pobjCI->mapDISPLAY);
				// disable options
				HWND hwCHK = GetDlgItem(hwndDlg, IDC_CI_CHK_ALSO_AC);
				EnableWindow(hwCHK, FALSE);
				ShowWindow(hwCHK, SW_SHOW);
				hwCHK = GetDlgItem(hwndDlg, IDC_CI_CHK_ALSO_FL);
				EnableWindow(hwCHK, FALSE);
				ShowWindow(hwCHK, SW_SHOW);
			}
			else if (wsCategory == L"AutoCompletion") {
				_populate_file_cbx(hwndDlg, pobjCI->mapAC, pobjCI->mapDISPLAY);
				// disable options
				HWND hwCHK = GetDlgItem(hwndDlg, IDC_CI_CHK_ALSO_AC);
				ShowWindow(hwCHK, SW_HIDE);
				hwCHK = GetDlgItem(hwndDlg, IDC_CI_CHK_ALSO_FL);
				ShowWindow(hwCHK, SW_HIDE);
			}
			else if (wsCategory == L"FunctionList") {
				_populate_file_cbx(hwndDlg, pobjCI->mapFL, pobjCI->mapDISPLAY);
				// disable options
				HWND hwCHK = GetDlgItem(hwndDlg, IDC_CI_CHK_ALSO_AC);
				ShowWindow(hwCHK, SW_HIDE);
				hwCHK = GetDlgItem(hwndDlg, IDC_CI_CHK_ALSO_FL);
				ShowWindow(hwCHK, SW_HIDE);
			}
			else if (wsCategory == L"Theme") {
				_populate_file_cbx(hwndDlg, pobjCI->vThemeFiles);
				// disable options
				HWND hwCHK = GetDlgItem(hwndDlg, IDC_CI_CHK_ALSO_AC);
				ShowWindow(hwCHK, SW_HIDE);
				hwCHK = GetDlgItem(hwndDlg, IDC_CI_CHK_ALSO_FL);
				ShowWindow(hwCHK, SW_HIDE);
			}
			return true;
		}
	}
	return false;
	case WM_DESTROY:
		DestroyWindow(hwndDlg);
		return true;
	}
	return false;
}
#pragma warning(pop)

void _populate_file_cbx(HWND hwndDlg, std::vector<std::wstring>& vsList)
{
	::SendDlgItemMessage(hwndDlg, IDC_CI_COMBO_FILE, LB_RESETCONTENT, 0, 0);
	for (auto& str : vsList) {
		::SendDlgItemMessage(hwndDlg, IDC_CI_COMBO_FILE, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(str.data()));
	}
	::SendDlgItemMessage(hwndDlg, IDC_CI_COMBO_FILE, LB_SETCURSEL, 0, 0);
	return;
}

void _populate_file_cbx(HWND hwndDlg, std::map<std::wstring, std::wstring>& mapURLs, std::map<std::wstring, std::wstring>& mapDisplay)
{
	::SendDlgItemMessage(hwndDlg, IDC_CI_COMBO_FILE, LB_RESETCONTENT, 0, 0);
	for (const auto& pair : mapURLs) {
		auto key = pair.first;
		if (mapDisplay.count(key)) {
			::SendDlgItemMessage(hwndDlg, IDC_CI_COMBO_FILE, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(mapDisplay[key].data()));
		}
	}
	::SendDlgItemMessage(hwndDlg, IDC_CI_COMBO_FILE, LB_SETCURSEL, 0, 0);
	return;
}

std::wstring _get_tab_category_wstr(HWND hwndDlg, int idcTabCtrl)
{
	std::wstring wsCategory(256, L'\0');
	TCITEM tCatInfo = {};
	tCatInfo.mask = TCIF_TEXT;
	tCatInfo.pszText = const_cast<LPWSTR>(wsCategory.data());
	tCatInfo.cchTextMax = static_cast<int>(wsCategory.size());
	LRESULT selectedIndex = ::SendDlgItemMessage(hwndDlg, idcTabCtrl, TCM_GETCURSEL, 0, 0);
	if (selectedIndex != -1) {
		if (::SendDlgItemMessage(hwndDlg, idcTabCtrl, TCM_GETITEM, selectedIndex, reinterpret_cast<LPARAM>(&tCatInfo))) {
			wsCategory.resize(lstrlen(wsCategory.c_str()));
			return wsCategory;
		}
	}
	return L"";
}
