#pragma once
#include "PluginDefinition.h"
#include "pcjHelper.h"
#include "menuCmdID.h"
#include <string>

class NppMetaInfo {
public:
	// constructor
	NppMetaInfo(void);

	// Run this in the constructor for any class that uses it
	// (internally tracks whether it's been populated already, and doesn't duplicate effort)
	void populate(void);

	NppData& hwnd;

	struct {
		std::wstring
			app,
			appDataEquiv,
			appThemes,
			appExePath,
			cfg,
			cfgPluginConfig,
			cfgPluginConfigMyDir,
			cfgUdl,
			cfgFunctionList,
			cfgAutoCompletion,
			cfgThemes;
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
};

// make sure everyone who can see this class can see the global instance
extern NppMetaInfo gNppMetaInfo;
