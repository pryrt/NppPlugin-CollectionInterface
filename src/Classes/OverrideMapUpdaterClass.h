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

	// add an <association> tag for a given UDL
	tinyxml2::XMLElement* OverrideMapUpdater::add_udl_assoc(std::wstring wsFilename, std::wstring wsUDLname);
	tinyxml2::XMLElement* OverrideMapUpdater::add_udl_assoc(std::string sFilename, std::string sUDLname);

	// add <association> tags for each filename,udl pair in the map
	std::vector<tinyxml2::XMLElement*> OverrideMapUpdater::add_udl_assoc(std::map<std::wstring, std::wstring> mwsUdls);
	std::vector<tinyxml2::XMLElement*> OverrideMapUpdater::add_udl_assoc(std::map<std::string, std::string> msUdls);

	// save the underlying overrideMap.xml
	tinyxml2::XMLError OverrideMapUpdater::saveFile(void) { return pOverrideMapXML->SaveFile(sOverMapPath().c_str()); }
	tinyxml2::XMLError OverrideMapUpdater::saveFile(std::string sNewPath) { return pOverrideMapXML->SaveFile(sNewPath.c_str()); }
	tinyxml2::XMLError OverrideMapUpdater::saveFile(std::wstring wsNewPath) { return pOverrideMapXML->SaveFile(pcjHelper::wstring_to_utf8(wsNewPath).c_str()); }

	// getters
	std::wstring wsOverMapPath(void) { return _wsOverMapPath; }
	std::string sOverMapPath(void) { return pcjHelper::wstring_to_utf8(_wsOverMapPath); }

private:
	////////////////
	// methods
	////////////////

	// look for an element, based on {Parent, FirstChild, or both} which is of a specific ElementType, having a specific AttributeName with specific AttributeValue
	tinyxml2::XMLElement* _find_element_with_attribute_value(tinyxml2::XMLElement* pParent, tinyxml2::XMLElement* pFirst, std::string sElementType, std::string sAttributeName, std::string sAttributeValue, bool caseSensitive);

	// compares the XMLError result to XML_SUCCESS, and returns a TRUE boolean to indicate failure
	//		p_doc defaults to nullptr, wsFilePath to L""
	bool OverrideMapUpdater::_xml_check_result(tinyxml2::XMLError a_eResult, tinyxml2::XMLDocument* p_doc = nullptr, std::wstring wsFilePath = std::wstring(L""));

	////////////////
	// properties
	////////////////

	// paths
	std::wstring _wsOverMapPath;		// <cfg>\functionList\overrideMap.xml path, or <exe> version
	std::wstring _wsAppOverMapPath;		// <exe>\functionList\overrideMap.xml path

	// tinyxml2
	tinyxml2::XMLDocument* pOverrideMapXML;
	tinyxml2::XMLElement* pRoot, * pFunctionList, * pAssociationMap;
};
