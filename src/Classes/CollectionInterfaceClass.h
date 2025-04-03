#pragma once
#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <windows.h>
#include <wininet.h>
#include <pathcch.h>
#include "PluginDefinition.h"


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
	//std::vector<char> downloadFileInMemory(const std::string& url);
	std::vector<char> downloadFileInMemory(const std::wstring& url);
	//bool downloadFileToDisk(const std::string& url, const std::string& path);
	//bool downloadFileToDisk(const std::wstring& url, const std::string& path);
	//bool downloadFileToDisk(const std::string& url, const std::wstring& path);
	bool downloadFileToDisk(const std::wstring& url, const std::wstring& path);
	void getListsFromJson(void);
	// getter methods
	std::wstring nppCfgDir(void) { return _nppCfgDir; };
	std::wstring nppCfgUdlDir(void) { return _nppCfgUdlDir; };
	std::wstring nppCfgFunctionListDir(void) { return _nppCfgFunctionListDir; };
	std::wstring nppCfgAutoCompletionDir(void) { return _nppCfgAutoCompletionDir; };
	std::wstring nppCfgThemesDir(void) { return _nppCfgThemesDir; };

private:
	std::string _xml_unentity(const std::string& text);

	// Npp Metadata
	void _populateNppDirs(void);
	std::wstring
		_nppCfgDir,
		_nppCfgUdlDir,
		_nppCfgFunctionListDir,
		_nppCfgAutoCompletionDir,
		_nppCfgThemesDir;

	HWND _hwndNPP;
};
