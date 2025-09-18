#pragma once
#include "PluginDefinition.h"
#include "pcjHelper.h"
#include "menuCmdID.h"
#include <string>

class NppMetaInfo {
public:
	// constructor
	NppMetaInfo(std::wstring wsPluginName);

	// Run this in the constructor for any class that uses it
	// (internally tracks whether it's been populated already, and doesn't duplicate effort)
	void populate(void);

	NppData& hwnd;

	struct {
		std::wstring
			app,						// directory where the executable is found
			appDataEquiv,				// %AppData% or .app, ignoring Cloud or -settingsDir
			appThemes,					// .app\Themes\ subfolder (alternate themes always under .app\ directory)
			appExePath,					// .app\notepad++.exe == "path\to\notepad++.exe" (includes .exe file)
			cfg,						// SettingsDir >> CloudDir >> AppData(equiv)
			cfgPluginConfig,			// .appDataEquiv\plugins\Config
			cfgPluginConfigMyDir,		// .appDataEquiv\plugins\Config\wsPluginName
			cfgUdl,						// .cfg\userDefinedLangs\ subdir (SettingsDir >> Cloud Dir >> Portable >> AppData(equiv))
			cfgFunctionList,			// .appDataEquiv\functionList\ subdir (skip Cloud or -settingsDir)
			cfgAutoCompletion,			// .app\autoCompletion\ subdir
			cfgThemes;					// .cfg\Themes\ subfoloder (SettingsDir >> Cloud Dir >> Portable >> AppData(equiv))
	} dir;

	////////////////////////////////
	// NPP VERSION and comparisons
	////////////////////////////////
	LRESULT ver; // npp version

	// compare NppVersion vs desired threshold
	bool isNppVerAtLeast(LRESULT maj, LRESULT min, LRESULT sub, LRESULT rev) { return ver >= verDotToLL(maj, min, sub, rev); }
	bool isNppVerAtLeast(LRESULT verThreshold) { return ver >= verThreshold; }
	bool isNppVerNewerThan(LRESULT maj, LRESULT min, LRESULT sub, LRESULT rev) { return ver > verDotToLL(maj, min, sub, rev); }
	bool isNppVerNewerThan(LRESULT verThreshold) { return ver > verThreshold; }
	bool isNppVerNoMoreThan(LRESULT maj, LRESULT min, LRESULT sub, LRESULT rev) { return ver <= verDotToLL(maj, min, sub, rev); }
	bool isNppVerNoMoreThan(LRESULT verThreshold) { return ver <= verThreshold; }
	bool isNppVerOlderThan(LRESULT maj, LRESULT min, LRESULT sub, LRESULT rev) { return ver < verDotToLL(maj, min, sub, rev); }
	bool isNppVerOlderThan(LRESULT verThreshold) { return ver < verThreshold; }

	// converts maj.min.sub.rev to N++-style LRESULT
	LRESULT verDotToLL(LRESULT maj, LRESULT min, LRESULT sub, LRESULT rev) { return (maj << 16) | (100 * min + 10 * sub + rev); }



private:
	bool _isInitialized;
	std::wstring _askSettingsDir(void);
	std::wstring _wsPluginName;
};

// make sure everyone who can see this class can see the global instance
extern NppMetaInfo gNppMetaInfo;
