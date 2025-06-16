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
#include <Shlwapi.h>


HWND g_hwndCIDlg = nullptr, g_hwndCIHlpDlg = nullptr;
CollectionInterface* pobjCI;
void _populate_file_cbx(HWND hwndDlg, std::vector<std::wstring>& vsList);
void _populate_file_cbx(HWND hwndDlg, std::map<std::wstring, std::wstring>& mapURLs, std::map<std::wstring, std::wstring>& mapDisplay);
std::wstring _get_tab_category_wstr(HWND hwndDlg, int idcTabCtrl);

// for tab control in DarkMode:
bool g_IsDarkMode = false;
const UINT_PTR g_ci_dark_subclass = 118;	// Don uses 42 for his DarkMode subclassing; the id+callback must be unique, but I'll use my own "magic number"
static LRESULT CALLBACK cbTabSubclass(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
NppDarkMode::Colors myColors = { 0 };
NppDarkMode::Brushes myBrushes(myColors);
NppDarkMode::Pens myPens(myColors);


#pragma warning(push)
#pragma warning(disable: 4100)
INT_PTR CALLBACK ciDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	static bool didDownload = false;
	switch (uMsg) {
		case WM_INITDIALOG:
		{
			// store hwnd
			g_hwndCIDlg = hwndDlg;
			HWND hParent = GetParent(hwndDlg);

			// Find Center and then position the window:
			// find App center
			RECT rc;
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
			// ::SetWindowPos() moved to the end of the WM_INITDIALOG, _after_ the dark-mode changes, etc.

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
			Edit_SetText(GetDlgItem(hwndDlg, IDC_CI_PROGRESSLBL), L"READY");

			pobjCI = new CollectionInterface(hParent);
			if (!pobjCI->areListsPopulated()) {
				EndDialog(hwndDlg, 0);
				DestroyWindow(hwndDlg);
				g_hwndCIDlg = nullptr;
				delete pobjCI;
				pobjCI = NULL;
				return true;
			}
			_populate_file_cbx(hwndDlg, pobjCI->mapUDL, pobjCI->mapDISPLAY);

			// Dark Mode Subclass and Theme: needs to go _after_ all the controls have been initialized
			LRESULT nppVersion = ::SendMessage(nppData._nppHandle, NPPM_GETNPPVERSION, 1, 0);	// HIWORD(nppVersion) = major version; LOWORD(nppVersion) = zero-padded minor (so 8|500 will come after 8|410)
			LRESULT darkdialogVersion = MAKELONG(540, 8);		// NPPM_GETDARKMODECOLORS requires 8.4.1 and NPPM_DARKMODESUBCLASSANDTHEME requires 8.5.4
			LRESULT localsubclassVersion = MAKELONG(810, 8);	// from 8.540 to 8.810 (at least), need to do local subclassing because of tab control
			g_IsDarkMode = (bool)::SendMessage(nppData._nppHandle, NPPM_ISDARKMODEENABLED, 0, 0);
			if (g_IsDarkMode && (nppVersion >= darkdialogVersion)) {
				::SendMessage(nppData._nppHandle, NPPM_GETDARKMODECOLORS, sizeof(NppDarkMode::Colors), reinterpret_cast<LPARAM>(&myColors));
				myBrushes.change(myColors);
				myPens.change(myColors);

				// in older N++, need to subclass the tab control myself, because NPPM_DARKMODESUBCLASSANDTHEME doesn't apply to SysTabControl32 from 8.5.4 (when introduced) to 8.8.1 (at least); hopefully, 8.8.2 will have that fixed
				if (nppVersion <= localsubclassVersion) {
					::SetWindowSubclass(GetDlgItem(g_hwndCIDlg, IDC_CI_TABCTRL), cbTabSubclass, g_ci_dark_subclass, 0);
				}

				// For the rest, follow Notpead++ DarkMode settings
				::SendMessage(nppData._nppHandle, NPPM_DARKMODESUBCLASSANDTHEME, static_cast<WPARAM>(NppDarkMode::dmfInit), reinterpret_cast<LPARAM>(g_hwndCIDlg));
			}

			// Finally, show the dialog using SetWindowPos()
			::SetWindowPos(hwndDlg, HWND_TOP, x, y, (dlgRect.right - dlgRect.left), (dlgRect.bottom - dlgRect.top), SWP_SHOWWINDOW);
		}

		return true;
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDOK:
				case IDC_CI_BTN_DONE:
				{
					if (didDownload) {
						int ans = ::MessageBox(hwndDlg, L"Would you like to restart now?", L"Restart Needed", MB_YESNOCANCEL);
						switch (ans) {
							case IDYES:
							{
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

								break;
							}
							case IDNO:
							{
								break;
							}
							case IDCANCEL:
							{
								return true;
							}
						}
					}
					// intentionally fall through to IDCANCEL so it exits the dialog
				}
				case IDCANCEL:
				{
					EndDialog(hwndDlg, 0);
					DestroyWindow(hwndDlg);
					g_hwndCIDlg = nullptr;
					delete pobjCI;
					pobjCI = NULL;
					return true;
				}
				case IDC_CI_COMBO_FILE:
				{
					if (HIWORD(wParam) == LBN_SELCHANGE) {
						std::vector<int> vBuf;
						LRESULT selectionCount = ::SendMessage(reinterpret_cast<HWND>(lParam), LB_GETSELCOUNT, 0, 0);
						if (selectionCount == LB_ERR) {	// gives error on single-selection
							selectionCount = 1;
							vBuf.resize(selectionCount);
							LRESULT selectedIndex = ::SendMessage(reinterpret_cast<HWND>(lParam), LB_GETCURSEL, 0, 0);
							vBuf[0] = static_cast<int>(selectedIndex);
						}
						else {
							vBuf.resize(selectionCount);
							LRESULT stat = ::SendMessage(reinterpret_cast<HWND>(lParam), LB_GETSELITEMS, selectionCount, reinterpret_cast<LPARAM>(vBuf.data()));
							if (stat == LB_ERR) {
								for (int b = 0; b < selectionCount; b++)
									vBuf[b] = static_cast<int>(stat);
							}
						}
						// start with assuming disabled checkboxes
						bool anyAC = false, anyFL = false;
						// then go through each selection item, and update flag as needed
						for (auto selectedIndex: vBuf) {
							if (selectedIndex != LB_ERR) {
								LRESULT needLen = ::SendMessage(reinterpret_cast<HWND>(lParam), LB_GETTEXTLEN, selectedIndex, 0);
								std::wstring wsFilename(needLen, 0);
								::SendMessage(reinterpret_cast<HWND>(lParam), LB_GETTEXT, selectedIndex, reinterpret_cast<LPARAM>(wsFilename.data()));
								//::MessageBox(NULL, wsFilename.c_str(), L"Which File:", MB_OK);

								// look at pobjCI->revDISPLAY[]
								std::wstring ws_id_name = (pobjCI->revDISPLAY.count(wsFilename)) ? pobjCI->revDISPLAY[wsFilename] : L"!!DoesNotExist!!";
								anyAC |= static_cast<bool>(pobjCI->mapAC.count(ws_id_name));
								anyFL |= static_cast<bool>(pobjCI->mapFL.count(ws_id_name));
							}
						}
						EnableWindow(GetDlgItem(hwndDlg, IDC_CI_CHK_ALSO_AC), anyAC);
						EnableWindow(GetDlgItem(hwndDlg, IDC_CI_CHK_ALSO_FL), anyFL);

						// reset progress bar
						::SendDlgItemMessage(hwndDlg, IDC_CI_PROGRESSBAR, PBM_SETPOS, 0, 0);
						Edit_SetText(GetDlgItem(hwndDlg, IDC_CI_PROGRESSLBL), L"READY");
					}
					return true;
				}
				case IDC_CI_BTN_DOWNLOAD:
				{
					std::wstring wsCategory = _get_tab_category_wstr(hwndDlg, IDC_CI_TABCTRL);
					HWND hwLBFile = GetDlgItem(hwndDlg, IDC_CI_COMBO_FILE);

					// prepare for N selections
					std::vector<int> vBuf;
					LRESULT selectionCount = ::SendMessage(hwLBFile, LB_GETSELCOUNT, 0, 0);
					if (selectionCount == LB_ERR) {	// gives error on single-selection
						selectionCount = 1;
						vBuf.resize(selectionCount);
						LRESULT selectedIndex = ::SendMessage(hwLBFile, LB_GETCURSEL, 0, 0);
						vBuf[0] = static_cast<int>(selectedIndex);
					}
					else {
						vBuf.resize(selectionCount);
						LRESULT stat = ::SendMessage(hwLBFile, LB_GETSELITEMS, selectionCount, reinterpret_cast<LPARAM>(vBuf.data()));
						if (stat == LB_ERR) {
							for (int b = 0; b < selectionCount; b++)
								vBuf[b] = static_cast<int>(stat);
						}
					}

					// loop through each index in the buffer, and download as needed
					std::map<std::wstring, std::wstring> mapUacDelayed;
					for(auto selectedFileIndex: vBuf) {
						if (selectedFileIndex == LB_ERR) {
							::MessageBox(NULL, L"Could not understand name selection; sorry", L"Download Error", MB_ICONERROR);
							return true;
						}

						LRESULT needFileLen = ::SendMessage(hwLBFile, LB_GETTEXTLEN, selectedFileIndex, 0);
						if (needFileLen == LB_ERR) {
							::MessageBox(NULL, L"Could not understand name selection; sorry", L"Download Error", MB_ICONERROR);
							return true;
						}
						std::wstring wsFilename(needFileLen, 0);
						::SendMessage(hwLBFile, LB_GETTEXT, selectedFileIndex, reinterpret_cast<LPARAM>(wsFilename.data()));

						// update progress bar
						::SendDlgItemMessage(hwndDlg, IDC_CI_PROGRESSBAR, PBM_SETPOS, 0, 0);
						Edit_SetText(GetDlgItem(hwndDlg, IDC_CI_PROGRESSLBL), L"Downloading... 0%");

						int total = 1;
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
								size_t posLastSlash = alsoDownload[L"AC"][L"URL"].rfind(L'/');
								std::wstring acName = (posLastSlash == std::wstring::npos) ? (ws_id_name + L".xml") : (alsoDownload[L"AC"][L"URL"].substr(posLastSlash + 1));
								alsoDownload[L"AC"][L"PATH"] = pobjCI->nppCfgAutoCompletionDir() + L"\\" + acName;
								extraWritable[L"AC"] = pobjCI->isAutoCompletionDirWritable();
								total++;
							}

							// if cjkFL, then also download FL
							isCHK = BST_CHECKED == IsDlgButtonChecked(hwndDlg, IDC_CI_CHK_ALSO_FL);
							isEN = IsWindowEnabled(GetDlgItem(hwndDlg, IDC_CI_CHK_ALSO_FL));
							if (isEN && isCHK && pobjCI->mapFL.count(ws_id_name)) {
								alsoDownload[L"FL"][L"URL"] = pobjCI->mapFL[ws_id_name];
								alsoDownload[L"FL"][L"PATH"] = pobjCI->nppCfgFunctionListDir() + L"\\" + ws_id_name + L".xml";
								extraWritable[L"FL"] = pobjCI->isFunctionListDirWritable();
								total++;
							}
						}
						else if (wsCategory == L"AutoCompletion") {
							size_t posLastSlash = std::wstring::npos;
							std::wstring acName = ws_id_name + L".xml";
							if (pobjCI->mapAC.count(ws_id_name)) {
								wsURL = pobjCI->mapAC[ws_id_name];
								posLastSlash = wsURL.rfind(L'/');
								if (posLastSlash != std::wstring::npos) {
									acName = wsURL.substr(posLastSlash + 1);
								}
							}

							wsPath = pobjCI->nppCfgAutoCompletionDir() + L"\\" + acName;
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

						int count = 0;
						if (isWritable) {
							// download directly to the final destination
							didDownload |= pobjCI->downloadFileToDisk(wsURL, wsPath);
							std::wstring msg = L"Downloaded to " + wsPath;
							//::MessageBox(hwndDlg, msg.c_str(), L"Download Successful", MB_OK);
							count++;
						}
						else {
							// check if it needs to be overwritten before elevating permissions
							if (pobjCI->ask_overwrite_if_exists(wsPath)) {
								mapUacDelayed[wsURL] = wsPath;
								total--;
							}
							else {
								total--;
							}
						}

						// update progress bar
						wchar_t wcDLPCT[256];
						if (total < 1) total = 1;
						swprintf_s(wcDLPCT, L"Downloading %d%%", 100 * count / total);
						if (didDownload) {
							::SendDlgItemMessage(hwndDlg, IDC_CI_PROGRESSBAR, PBM_SETPOS, 100 * count / total, 0);
							Edit_SetText(GetDlgItem(hwndDlg, IDC_CI_PROGRESSLBL), wcDLPCT);
						}

						// also download AC and FL, if applicable
						std::vector<std::wstring> xtra = { L"AC", L"FL" };
						for (auto category : xtra) {
							if (alsoDownload.count(category)) {
								std::wstring xURL = alsoDownload[category][L"URL"];
								std::wstring xPath = alsoDownload[category][L"PATH"];
								if (extraWritable[category]) {
									pobjCI->downloadFileToDisk(xURL, xPath);
									count++;
									didDownload = true;
								}
								else {
									if (pobjCI->ask_overwrite_if_exists(xPath)) {
										mapUacDelayed[xURL] = xPath;
										total--;
									}
									else {
										total--;
									}
								}
							}
							// update progress bar
							if (total < 1) total = 1;
							swprintf_s(wcDLPCT, L"Downloading %d%%", 100 * count / total);
							if (didDownload) {
								::SendDlgItemMessage(hwndDlg, IDC_CI_PROGRESSBAR, PBM_SETPOS, 100 * count / total, 0);
								Edit_SetText(GetDlgItem(hwndDlg, IDC_CI_PROGRESSLBL), wcDLPCT);
							}
						}

						// Final update of progress bar: 100%
						if (didDownload) {
							::SendDlgItemMessage(hwndDlg, IDC_CI_PROGRESSBAR, PBM_SETPOS, 100, 0);
							swprintf_s(wcDLPCT, L"Downloading %d%% [DONE]", 100);
							Edit_SetText(GetDlgItem(hwndDlg, IDC_CI_PROGRESSLBL), wcDLPCT);
						}
						else {
							::SendDlgItemMessage(hwndDlg, IDC_CI_PROGRESSBAR, PBM_SETPOS, 0, 0);
							swprintf_s(wcDLPCT, L"Nothing to Download.  [DONE]");
							Edit_SetText(GetDlgItem(hwndDlg, IDC_CI_PROGRESSLBL), wcDLPCT);
						}
					}

					if (mapUacDelayed.size()) {
						int total = static_cast<int>(mapUacDelayed.size()) + 1;			// want one extra "slot" for the MOVE command
						int count = 0;
						std::wstring wsAsk = L"Cannot write the following files:";
						for (const auto& pair : mapUacDelayed) {
							wsAsk += std::wstring(L"\n") + pair.second;
						}
						wsAsk += L"\n\nI will download temporary files, and then try to copy them to the right location with elevated UAC permission.  (The OS may prompt you for UAC.)";
						int ans = ::MessageBox(hwndDlg, wsAsk.c_str(), L"Need Directory Permission", MB_OKCANCEL);
						if (ans == IDOK) {
							wchar_t wcDLPCT[256];
							swprintf_s(wcDLPCT, L"Downloading %d%%", 100 * count / total);
							Edit_SetText(GetDlgItem(hwndDlg, IDC_CI_PROGRESSLBL), wcDLPCT);

							std::wstring args = L"/C ";

							for (const auto& pair : mapUacDelayed) {
								++count;
								std::wstring tmpPath = pobjCI->getWritableTempDir() + L"\\~$TMPFILE.DOWNLOAD.PRYRT." + std::to_wstring(count);
								pobjCI->downloadFileToDisk(pair.first, tmpPath);
								didDownload = true;
								args += L"MOVE /Y \"" + tmpPath + L"\" \"" + pair.second + L"\" & ";

								::SendDlgItemMessage(hwndDlg, IDC_CI_PROGRESSBAR, PBM_SETPOS, 100 * count / total, 0);
								swprintf_s(wcDLPCT, L"Downloading %d%%", 100 * count / total);
								Edit_SetText(GetDlgItem(hwndDlg, IDC_CI_PROGRESSLBL), wcDLPCT);
							}

							//::MessageBox(hwndDlg, (std::wstring(L"cmd.exe ") + args).c_str(), L"TODO: UAC Command", MB_OK);
							ShellExecute(hwndDlg, L"runas", L"cmd.exe", args.c_str(), NULL, SW_SHOWMINIMIZED);

							::SendDlgItemMessage(hwndDlg, IDC_CI_PROGRESSBAR, PBM_SETPOS, 100, 0);
							swprintf_s(wcDLPCT, L"Downloading %d%% [DONE]", 100);
							Edit_SetText(GetDlgItem(hwndDlg, IDC_CI_PROGRESSLBL), wcDLPCT);
						}
					}

					return true;
				}
				case IDC_CI_HELPBTN:
				{
					showCIDownloadHelp();
					return true;
				}
				default:
				{
					return false;
				}
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
				// reset progress bar
				::SendDlgItemMessage(hwndDlg, IDC_CI_PROGRESSBAR, PBM_SETPOS, 0, 0);
				Edit_SetText(GetDlgItem(hwndDlg, IDC_CI_PROGRESSLBL), L"READY");

				// done with SELCHANGE
				return true;
			}
			return false;
		}
		case WM_DESTROY:
			//DestroyWindow(hwndTab);
			DestroyWindow(hwndDlg);
			g_hwndCIDlg = nullptr;
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



INT_PTR CALLBACK cidlHelpDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static std::wstring wsHelpText(L"");
	if (wsHelpText == L"") {
		wsHelpText += L"Pick the category's tab: UDL, AutoCompletion, FunctionList, or Theme\r\n\r\n";
		wsHelpText += L"It will populate the list with the names for each of the files; you can select one from that list (using the scrollbar as needed).\r\n\r\n";
		wsHelpText += L"Since User Defined Language (UDL) definitions can have associated AutoCompletion or FunctionList definitions, if one or both of those types of file are associated with the UDL you have selected, you will be able to choose, using the appropriate checkbox, whether to also download the associated AutoCompletion and/or FunctionList at the same time.  If you are in the AutoCompletion or FunctionList tabs, it is assumed that you _just_ want to download that specific file, not the associated UDL.\r\n\r\n";
		wsHelpText += L"Themes are independent of the UDL and associated files, so you just select the theme you want to download.\r\n\r\n";
		wsHelpText += L"Click DOWNLOAD to start the download of the selected file (and optionally, any associated files that were chosen).\r\n\r\n";
		wsHelpText += L"If you do not have permission to write in the necessary directory for a given file, a MessageBox will inform you of this, and if you do not CANCEL that file, Windows will ask for elevated UAC permission. (It will ask once per file that needs the extra permission.)\r\n\r\n";
		wsHelpText += L"You will need to restart Notepad++ for the new file(s) to be available for use in Notepad++.  If you downloaded at least one file, the CollectionInterface Download will ask if you want it to restart Notepad++ for you.";
	}

	switch (uMsg) {
		case WM_INITDIALOG:
		{
			// populate with help text:
			HWND hEdit = ::GetDlgItem(hwndDlg, IDC_CIDH_BIGTEXT);
			::SetWindowText(hEdit, wsHelpText.c_str());

			// store hwnd
			g_hwndCIHlpDlg = hwndDlg;

			// determine dark mode
			g_IsDarkMode = (bool)::SendMessage(nppData._nppHandle, NPPM_ISDARKMODEENABLED, 0, 0);
			if (g_IsDarkMode) {
				::SendMessage(nppData._nppHandle, NPPM_DARKMODESUBCLASSANDTHEME, static_cast<WPARAM>(NppDarkMode::dmfInit), reinterpret_cast<LPARAM>(g_hwndCIHlpDlg));
			}

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
			rc.left -= static_cast<int>(lParam);
			rc.left -= static_cast<int>(wParam);
			rc.left += static_cast<int>(lParam);
			rc.left += static_cast<int>(wParam);

			// and position dialog
			RECT dlgRect;
			::GetClientRect(hwndDlg, &dlgRect);
			int x = center.x - (dlgRect.right - dlgRect.left) / 2;
			int y = center.y - (dlgRect.bottom - dlgRect.top) / 2;
			::SetWindowPos(hwndDlg, HWND_TOP, x, y, (dlgRect.right - dlgRect.left), (dlgRect.bottom - dlgRect.top), SWP_SHOWWINDOW);
		}

		return true;
		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDCANCEL:
				case IDOK:
					EndDialog(hwndDlg, 0);
					DestroyWindow(hwndDlg);
					return true;
			}
			return false;
		case WM_CLOSE:
			EndDialog(hwndDlg, 0);
			DestroyWindow(hwndDlg);
			return true;
		case WM_SIZE:
		{
			//// int newWidth = LOWORD(lParam);
			//// int newHeight = HIWORD(lParam);
			//// 
			//// // Resize the edit control, adjust position and size as needed
			//// HWND hEdit = ::GetDlgItem(hwndDlg, IDC_CIDH_BIGTEXT);
			//// SetWindowPos(
			//// 	hEdit,
			//// 	nullptr,
			//// 	10, // X position
			//// 	10, // Y position
			//// 	newWidth - 20, // Width
			//// 	newHeight - 20, // Height
			//// 	SWP_NOZORDER
			//// );
			//// //::SendMessage(hEdit, EM_SETREADONLY, static_cast<WPARAM>(false), 0);
			//// //::SetWindowText(hEdit, L"Hello World");
			//// //::SendMessage(hEdit, EM_SETSEL, 0, 0);
			//// ::SetWindowText(hEdit, wsHelpText.c_str());
			//// //::SendMessage(hEdit, EM_SETREADONLY, static_cast<WPARAM>(true), 0);
			return true;
		}
		default:
			return false;
	}
}

static LRESULT CALLBACK cbTabSubclass(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR /*dwRefData*/)
{
	if (!g_IsDarkMode) return false;
	switch (uMsg)
	{
		case WM_ERASEBKGND:
		{
			return TRUE;
		}

		case WM_PAINT:
		{
			LONG_PTR dwStyle = GetWindowLongPtr(hWnd, GWL_STYLE);
			if ((dwStyle & TCS_BUTTONS) || (dwStyle & TCS_VERTICAL))
			{
				break;
			}

			PAINTSTRUCT ps{};
			HDC hdc = ::BeginPaint(hWnd, &ps);
			::FillRect(hdc, &ps.rcPaint, myBrushes.dlgBackground);

			auto holdPen = static_cast<HPEN>(::SelectObject(hdc, myPens.edgePen));

			HRGN holdClip = CreateRectRgn(0, 0, 0, 0);
			if (1 != GetClipRgn(hdc, holdClip))
			{
				DeleteObject(holdClip);
				holdClip = nullptr;
			}

			HFONT hFont = reinterpret_cast<HFONT>(SendMessage(hWnd, WM_GETFONT, 0, 0));
			auto hOldFont = SelectObject(hdc, hFont);

			POINT ptCursor{};
			::GetCursorPos(&ptCursor);
			ScreenToClient(hWnd, &ptCursor);

			int nTabs = TabCtrl_GetItemCount(hWnd);

			int nSelTab = TabCtrl_GetCurSel(hWnd);
			for (int i = 0; i < nTabs; ++i)
			{
				RECT rcItem{};
				TabCtrl_GetItemRect(hWnd, i, &rcItem);
				RECT rcFrame = rcItem;

				RECT rcIntersect{};
				if (IntersectRect(&rcIntersect, &ps.rcPaint, &rcItem))
				{
					bool bHot = PtInRect(&rcItem, ptCursor);
					bool isSelectedTab = (i == nSelTab);

					HRGN hClip = CreateRectRgnIndirect(&rcItem);

					SelectClipRgn(hdc, hClip);

					SetTextColor(hdc, (bHot || isSelectedTab) ? myColors.text : myColors.darkerText);

					::InflateRect(&rcItem, -1, -1);
					rcItem.right += 1;

					// for consistency getBackgroundBrush()
					// would be better, than getCtrlBackgroundBrush(),
					// however default getBackgroundBrush() has same color
					// as getDlgBackgroundBrush()
					::FillRect(hdc, &rcItem, isSelectedTab ? myBrushes.dlgBackground : bHot ? myBrushes.hotBackground : myBrushes.ctrlBackground);

					SetBkMode(hdc, TRANSPARENT);

					wchar_t label[MAX_PATH]{};
					TCITEM tci{};
					tci.mask = TCIF_TEXT;
					tci.pszText = label;
					tci.cchTextMax = MAX_PATH - 1;

					::SendMessage(hWnd, TCM_GETITEM, i, reinterpret_cast<LPARAM>(&tci));

					RECT rcText = rcItem;
					if (isSelectedTab)
					{
						::OffsetRect(&rcText, 0, -1);
						::InflateRect(&rcFrame, 0, 1);
					}

					if (i != nTabs - 1)
					{
						rcFrame.right += 1;
					}

					::FrameRect(hdc, &rcFrame, myBrushes.edgeBrush);

					DrawText(hdc, label, -1, &rcText, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

					DeleteObject(hClip);

					SelectClipRgn(hdc, holdClip);
				}
			}

			SelectObject(hdc, hOldFont);

			SelectClipRgn(hdc, holdClip);
			if (holdClip)
			{
				DeleteObject(holdClip);
				holdClip = nullptr;
			}

			SelectObject(hdc, holdPen);

			EndPaint(hWnd, &ps);
			return 0;
		}

		case WM_NCDESTROY:
		{
			::RemoveWindowSubclass(hWnd, cbTabSubclass, uIdSubclass);
			break;
		}

	}
	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}
