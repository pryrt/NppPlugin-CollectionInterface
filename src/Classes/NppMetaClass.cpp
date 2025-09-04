#include "NppMetaClass.h"
#include <pathcch.h>
#include <shlwapi.h>

// Global NppMetaInfo object
NppMetaInfo gNppMetaInfo;

NppMetaInfo::NppMetaInfo(void)
	: hwnd(nppData)
{
	_isInitialized = false;
	dir = { L"", L"", L"", L"", L"", L"", L"", L"", L"", L"", L"" };
	ver = 0;
	return;
}

// Run this in the constructor for any class that uses it
// (internally tracks whether it's been populated already, and doesn't duplicate effort)
void NppMetaInfo::populate(void)
{
	if (_isInitialized) return;

	////////////////////////////////
	// Get the Notepad++ version (HIWORD=maj, LOWORD=minor*100+sub*10+rev)
	////////////////////////////////
	ver = ::SendMessage(hwnd._nppHandle, NPPM_GETNPPVERSION, 1, 0);

	////////////////////////////////
	// Get the directory which stores the executable
	////////////////////////////////
	std::wstring exeDir(MAX_PATH, '\0');
	::SendMessage(hwnd._nppHandle, NPPM_GETNPPDIRECTORY, MAX_PATH, reinterpret_cast<LPARAM>(exeDir.data()));
	pcjHelper::delNull(exeDir);
	// and the excecutable file path (including `\notepad++.exe`)
	std::wstring exePath(MAX_PATH, '\0');
	LRESULT szExe = ::SendMessage(hwnd._nppHandle, NPPM_GETNPPFULLFILEPATH, 0, reinterpret_cast<LPARAM>(exePath.data()));
	if (szExe) {
		pcjHelper::delNull(exePath);
		dir.appExePath = exePath;
	}

	////////////////////////////////
	// Get the AppData or Portable folders, as being relative to the plugins/Config directory
	////////////////////////////////

	// %AppData%\Notepad++\Plugins\Config or equiv
	LRESULT sz = 1 + ::SendMessage(hwnd._nppHandle, NPPM_GETPLUGINSCONFIGDIR, 0, NULL);
	std::wstring pluginCfgDir(sz, '\0');
	::SendMessage(hwnd._nppHandle, NPPM_GETPLUGINSCONFIGDIR, sz, reinterpret_cast<LPARAM>(pluginCfgDir.data()));
	pcjHelper::delNull(pluginCfgDir);
	dir.cfgPluginConfig = pluginCfgDir;
	dir.cfgPluginConfigMyDir = pluginCfgDir + L"\\ConfigUpdater";

	// %AppData%\Notepad++\Plugins or equiv
	//		since it's removing the tail, it will never be longer than pluginCfgDir; since it's in-place, initialize with the first
	std::wstring pluginDir = pluginCfgDir;
	PathCchRemoveFileSpec(const_cast<PWSTR>(pluginDir.data()), pluginCfgDir.size());
	pcjHelper::delNull(pluginDir);

	// %AppData%\Notepad++ or equiv is what I'm really looking for
	// _nppCfgDir				#py# _nppConfigDirectory = os.path.dirname(os.path.dirname(notepad.getPluginConfigDir()))
	dir.cfg = pluginDir;
	PathCchRemoveFileSpec(const_cast<PWSTR>(dir.cfg.data()), pluginDir.size());
	pcjHelper::delNull(dir.cfg);

	// Save this version, because Cloud/SettingsDir will overwrite _npp.dir.cfg, but I will still need to be able to fall back to the AppData||Portable
	dir.appDataEquiv = dir.cfg;

	bool usesSettingsDir = false;
	if (isNppVerAtLeast(8, 8, 6, 0)) {
		// with N++ 8.8.6 and newer, the added message simplifies things to not need the commplicated _askSettingsDir() to extract -settingsDir from command line string
		sz = ::SendMessage(hwnd._nppHandle, NPPM_GETNPPSETTINGSDIRPATH, 0, 0); // get number of wchars
		if (sz) {
			std::wstring wsNppUserDir(sz + 1, '\0');
			LRESULT szGot = ::SendMessage(hwnd._nppHandle, NPPM_GETNPPSETTINGSDIRPATH, sz + 1, reinterpret_cast<LPARAM>(wsNppUserDir.data()));
			if (szGot == sz) {
				pcjHelper::delNull(wsNppUserDir);
				dir.cfg = wsNppUserDir;
			}
		}

		// if the _userDir is not AppData||NppDirectory, AND if cloud not enabled, then it is -settingsDir
		if (dir.cfg != dir.appDataEquiv) {
			if (!::SendMessage(hwnd._nppHandle, NPPM_GETSETTINGSONCLOUDPATH, 0, 0))
				usesSettingsDir = true;
		}
	}
	else
	{
		////////////////////////////////
		// Check for cloud and -settingsDir config-override locations
		////////////////////////////////
		sz = ::SendMessage(hwnd._nppHandle, NPPM_GETSETTINGSONCLOUDPATH, 0, 0); // get number of wchars in settings-on-cloud path (0 means settings-on-cloud is disabled)
		if (sz) {
			std::wstring wsCloudDir(sz + 1, '\0');
			LRESULT szGot = ::SendMessage(hwnd._nppHandle, NPPM_GETSETTINGSONCLOUDPATH, sz + 1, reinterpret_cast<LPARAM>(wsCloudDir.data()));
			if (szGot == sz) {
				pcjHelper::delNull(wsCloudDir);
				dir.cfg = wsCloudDir;
			}
		}

		// -settingsDir: if command-line option is enabled, use that directory for some config files
		std::wstring wsSettingsDir = _askSettingsDir();
		if (wsSettingsDir.length() > 0)
		{
			usesSettingsDir = true;
			dir.cfg = wsSettingsDir;
		}
	}

	////////////////////////////////
	// Now that we've got all the info, decide on final locations for Function List, UDL, and Themes;
	// AutoCompletion is always relative to executable directory
	////////////////////////////////

	// FunctionList: must be AppData or PortableDir, because FL does NOT work in Cloud or Settings directory
	// _nppCfgFunctionListDir	#py# _nppCfgFunctionListDirectory = os.path.join(_nppConfigDirectory, 'functionList')
	dir.cfgFunctionList = dir.appDataEquiv + L"\\functionList";

	// UDL: follows SettingsDir >> Cloud Dir >> Portable >> AppData
	// _nppCfgUdlDir			#py# _nppCfgUdlDirectory = os.path.join(_nppConfigDirectory, 'userDefineLangs')
	dir.cfgUdl = dir.cfg + L"\\userDefineLangs";

	// THEMES: does NOT follow SettingsDir (might be a N++ bug, but must live with what IS, not what SHOULD BE); DOES follow Cloud Dir >> Portable >> AppData
	// _nppCfgThemesDir			#py# _nppCfgThemesDirectory = os.path.join(_nppConfigDirectory, 'themes')
	dir.cfgThemes = (usesSettingsDir ? dir.appDataEquiv : dir.cfg) + L"\\themes";

	// AutoCompletion is _always_ relative to notepad++.exe, never in AppData or CloudConfig or SettingsDir
	// _nppCfgAutoCompletionDir	#py# _nppAppAutoCompletionDirectory = os.path.join(notepad.getNppDir(), 'autoCompletion')
	dir.cfgAutoCompletion = exeDir + L"\\autoCompletion";

	// also, want to save the app directory and the themes relative to the app (because themes can be found in _both_ locations)
	dir.app = exeDir;
	dir.appThemes = dir.app + L"\\themes";

	// Done with populate(), so safe to call it initialized
	_isInitialized = true;
	return;
}

// Parse the -settingsDir out of the current command line
//	FUTURE: if a future version of N++ includes PR#16946, then do an "if version>vmin, use new message" section in the code
std::wstring NppMetaInfo::_askSettingsDir(void)
{
	LRESULT sz = ::SendMessage(hwnd._nppHandle, NPPM_GETCURRENTCMDLINE, 0, 0);
	if (!sz) return L"";
	std::wstring strCmdLine(sz + 1, L'\0');
	LRESULT got = ::SendMessage(hwnd._nppHandle, NPPM_GETCURRENTCMDLINE, sz + 1, reinterpret_cast<LPARAM>(strCmdLine.data()));
	if (got != sz) return L"";
	pcjHelper::delNull(strCmdLine);

	std::wstring wsSettingsDir = L"";
	size_t p = 0;
	if ((p = strCmdLine.find(L"-settingsDir=")) != std::wstring::npos) {
		std::wstring wsEnd = L" ";
		// start by grabbing from after the = to the end of the string
		wsSettingsDir = strCmdLine.substr(p + 13, strCmdLine.length() - p - 13);
		if (wsSettingsDir[0] == L'"') {
			wsSettingsDir = wsSettingsDir.substr(1, wsSettingsDir.length() - 1);
			wsEnd = L"\"";
		}
		p = wsSettingsDir.find(wsEnd);
		if (p != std::wstring::npos) {
			// found the ending space or quote, so do everything _before_ that (need last position=p-1, which means a count of p)
			wsSettingsDir = wsSettingsDir.substr(0, p);
		}
		else if (wsEnd == L"\"") {
			// could not find end quote; should probably throw an error or something, but for now, just pretend I found nothing...
			return L"";
		}	// none found and looking for space means it found end-of-string, which is fine with the space separator
	}

	return wsSettingsDir;
}
