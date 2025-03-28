#pragma once
#include <string>
#include <vector>
#include <stdexcept>
#include <windows.h>
#include <wininet.h>

class CollectionInterface {
public:
	// Grabs the UDL List and Themes list from the collections, and populates the vector<wstring> lists
	CollectionInterface(void);

	// lists of files
	std::vector<std::wstring> vwsUDLFiles;
	std::vector<std::wstring> vwsACFiles;
	std::vector<std::wstring> vwsFLFiles;
	std::vector<std::wstring> vwsThemeFiles;

	std::vector<char> downloadFileInMemory(const std::string& url);
};
