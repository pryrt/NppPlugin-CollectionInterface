#pragma once
#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <windows.h>
#include <wininet.h>
#include <pathcch.h>
#include "PluginDefinition.h"
#include "NppMetaClass.h"

class CollectionInterface {
public:
	// Grabs the UDL List and Themes list from the collections, and populates the vector<wstring> lists
	CollectionInterface(HWND hwndNpp);

	// new structure is just a separate map for each id-to-URL
	std::map<std::wstring, std::wstring>
		mapUDL,			// [id_name] => UDL_URL
		mapAC,			// [id_name] => AC_URL
		mapFL,			// [id_name] => FL_URL
		mapDISPLAY,		// [id_name] => DISPLAY NAME
		revDISPLAY;		// [display_name] => id_name

	// Themes are just a simple list
	std::vector<std::wstring> vThemeFiles;

	// Methods
	std::vector<char> downloadFileInMemory(const std::wstring& url);
	bool downloadFileToDisk(const std::wstring& url, const std::wstring& path);
	bool getListsFromJson(void);

	// status methods
	bool isUdlDirWritable(void) { return _is_dir_writable(gNppMetaInfo.dir.cfgUdl); };
	bool isFunctionListDirWritable(void) { return _is_dir_writable(gNppMetaInfo.dir.cfgFunctionList); };
	bool isAutoCompletionDirWritable(void) { return _is_dir_writable(gNppMetaInfo.dir.cfgAutoCompletion); };
	bool isThemesDirWritable(void) { return _is_dir_writable(gNppMetaInfo.dir.cfgThemes); };
	bool areListsPopulated(void) { return _areListsPopulated; };

	// if the chosen directory isn't writable, need to be able to use a directory that _is_ writable
	//	as a TempDir, and then will need to use runas to copy from the TempDir to the real dir.
	std::wstring getWritableTempDir(void);

	// tests if it exists, and if so, ask if you want to overwrite or not
	bool ask_overwrite_if_exists(const std::wstring& path);

private:
	std::string _xml_unentity(const std::string& text);
	std::wstring& _wsDeleteTrailingNulls(std::wstring& text);
	bool _is_dir_writable(const std::wstring& path);
	BOOL _RecursiveCreateDirectory(std::wstring wsPath);
	bool _areListsPopulated;

	// TODO: see if trailing nulls, is-writable, and recursive-create can be replaced by pcjHelper instances
};
