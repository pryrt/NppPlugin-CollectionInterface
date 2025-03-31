#pragma once
#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <windows.h>
#include <wininet.h>

class CollectionInterface {
public:
	// Grabs the UDL List and Themes list from the collections, and populates the vector<wstring> lists
	CollectionInterface(void);

	// new structure is just a separate map for each id-to-URL
	std::map<std::string, std::string>
		mapUDL,			// [id_name] => UDL_URL
		mapAC,			// [id_name] => AC_URL
		mapFL,			// [id_name] => FL_URL
		mapDISPLAY,		// [id_name] => DISPLAY NAME
		revDISPLAY;		// [display_name] => id_name

	// Themes are just a simple list
	std::vector<std::string> vThemeFiles;

	// Methods
	std::vector<char> downloadFileInMemory(const std::string& url);
	void getListsFromJson(void);
private:
	std::string _xml_unentity(const std::string& text);
};
