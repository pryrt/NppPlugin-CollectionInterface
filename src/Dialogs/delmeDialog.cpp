#include "delmeDialog.h"
#include <Commctrl.h>

INT_PTR CALLBACK delDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
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
		::SetWindowPos(hwndDlg, HWND_TOP, x, y, (dlgRect.right - dlgRect.left), (dlgRect.bottom - dlgRect.top), SWP_SHOWWINDOW|SWP_NOSIZE);	// the SWP_NOSIZE will prevent resizing, so it just centers

		// populate tab bar: "INSERTITEM" means RightToLeft, because it inserts it before the earlier tab.
		std::wstring ws;
		TCITEM pop;
		pop.mask = TCIF_TEXT;
		ws = L"SOMETHING WITHOUT SUGAR"; pop.cchTextMax = static_cast<int>(ws.size()); pop.pszText = const_cast<LPWSTR>(ws.data());
		::SendDlgItemMessage(hwndDlg, IDC_TAB1, TCM_INSERTITEM, 0, reinterpret_cast<LPARAM>(&pop));
		ws = L"PEPSI FREE"; pop.cchTextMax = static_cast<int>(ws.size()); pop.pszText = const_cast<LPWSTR>(ws.data());
		::SendDlgItemMessage(hwndDlg, IDC_TAB1, TCM_INSERTITEM, 0, reinterpret_cast<LPARAM>(&pop));
		ws = L"TAB"; pop.cchTextMax = static_cast<int>(ws.size()); pop.pszText = const_cast<LPWSTR>(ws.data());
		::SendDlgItemMessage(hwndDlg, IDC_TAB1, TCM_INSERTITEM, 0, reinterpret_cast<LPARAM>(&pop));
		::SendDlgItemMessage(hwndDlg, IDC_TAB1, TCM_SETCURSEL, 0, 0);

		// populate tabfile list
		// IDC_LIST1
		::SendDlgItemMessage(hwndDlg, IDC_LIST1, LB_RESETCONTENT, 0, 0);
		::SendDlgItemMessage(hwndDlg, IDC_LIST1, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"<Pick File>"));
		::SendDlgItemMessage(hwndDlg, IDC_LIST1, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"<Pick Another>"));
		::SendDlgItemMessage(hwndDlg, IDC_LIST1, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"<Pick Again>"));

		// set progress bar
		::SendDlgItemMessage(hwndDlg, IDC_PROGRESS1, PBM_SETPOS, 55, 0);

	}
	return true;
	case WM_COMMAND:
	{
		switch (LOWORD(wParam)) {
		case IDCANCEL:
		case IDOK:
			EndDialog(hwndDlg, 0);
			DestroyWindow(hwndDlg);
			return true;
		}

		switch (lParam) {
		case 0:
			break;
		default:
			break;
		}
	}
	return true;
	case WM_NOTIFY :
	{
		if (wParam == IDC_TAB1) {
			if (((LPNMHDR)lParam)->code == TCN_SELCHANGE) {
				std::wstring ws(256, L'\0');
				TCITEM pop = {};
				pop.mask = TCIF_TEXT;
				pop.pszText = const_cast<LPWSTR>(ws.data());
				pop.cchTextMax = static_cast<int>(ws.size());
				LRESULT selectedIndex = ::SendDlgItemMessage(hwndDlg, IDC_TAB1, TCM_GETCURSEL, 0, 0);
				if (selectedIndex != -1) {
					if (::SendDlgItemMessage(hwndDlg, IDC_TAB1, TCM_GETITEM, selectedIndex, reinterpret_cast<LPARAM>(&pop))) {
						::MessageBox(hwndDlg, pop.pszText, L"Changed to tab", MB_OK);
					}
				}
			}
		}
	}
	return true;
	case WM_DESTROY:
	{
		DestroyWindow(hwndDlg);
	}
	return true;
	}
	return false;

}
