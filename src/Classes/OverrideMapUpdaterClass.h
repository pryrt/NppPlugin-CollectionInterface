#pragma once
#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <windows.h>
#include <wininet.h>
#include <pathcch.h>
#include <shlwapi.h>
#include "PluginDefinition.h"
#include "NppMetaClass.h"
#include "tinyxml2.h"
#include "pcjHelper.h"

class OverrideMapUpdater {
public:
	// Instantiate OverrideMapUpdater object
	OverrideMapUpdater(void);

	// Experiment with concepts needed, might eventually morph into actual behavior
	bool experiment(void);

	// add an <association> tag for a given UDL
	tinyxml2::XMLElement* OverrideMapUpdater::add_udl_assoc(std::wstring wsFilename, std::wstring wsUDLname);
	tinyxml2::XMLElement* OverrideMapUpdater::add_udl_assoc(std::string sFilename, std::string sUDLname);

	// add <association> tags for each filename,udl pair in the map
	std::vector<tinyxml2::XMLElement*> OverrideMapUpdater::add_udl_assoc(std::map<std::wstring, std::wstring> mwsUdls);
	std::vector<tinyxml2::XMLElement*> OverrideMapUpdater::add_udl_assoc(std::map<std::string, std::string> msUdls);

	// getters
	std::wstring wsOverMapPath(void) { return _wsOverMapPath; }
	std::string sOverMapPath(void) { return pcjHelper::wstring_to_utf8(_wsOverMapPath); }

private:
	// properties

	// paths
	std::wstring _wsOverMapPath;		// <cfg>\functionList\overrideMap.xml path, or <exe> version
	std::wstring _wsAppOverMapPath;		// <exe>\functionList\overrideMap.xml path

	// tinyxml2
	tinyxml2::XMLDocument* pOverrideMapXML;
	tinyxml2::XMLElement* pRoot, * pFunctionList, * pAssociationMap;
};
