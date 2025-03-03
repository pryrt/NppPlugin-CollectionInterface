#pragma once
#include <string>
#include <vector>

class CollectionInterface {
public:
	// Grabs the UDL List and Themes list from the collections, and populates the vector<wstring> lists
	CollectionInterface(void);

	// lists of files
	std::vector<std::wstring> vwsUDLFiles;
	std::vector<std::wstring> vwsACFiles;
	std::vector<std::wstring> vwsFLFiles;
	std::vector<std::wstring> vwsThemeFiles;
};
