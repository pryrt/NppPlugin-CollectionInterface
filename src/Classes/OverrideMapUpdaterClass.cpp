#include "OverrideMapUpdaterClass.h"

// Experiment with concepts needed, might eventually morph into actual behavior
OverrideMapUpdater::OverrideMapUpdater(void)
{
	gNppMetaInfo.populate();

	// 1. Pick correct file: "cfg" directory (Settings/Cloud/AppData) or "app" directory, whichever exists
	_wsOverMapPath = gNppMetaInfo.dir.cfgFunctionList + L"\\overrideMap.xml";
	_wsAppOverMapPath = gNppMetaInfo.dir.app + L"\\functionList\\overrideMap.xml";
	bool needWriteToAppDir = false;
	bool needToCreateFile = false;

	if (!PathFileExists(_wsOverMapPath.c_str())) {
		// since .cfgFunctionList\overrideMap.xml doesn't exist, try to copy from app dir instead
		if (PathFileExists(_wsAppOverMapPath.c_str())) {
			// check if writeable
			if (pcjHelper::is_dir_writable(gNppMetaInfo.dir.cfgFunctionList)) {
				// if writable, copy the app version to the cfg location
				CopyFile(_wsAppOverMapPath.c_str(), _wsOverMapPath.c_str(), TRUE);
			}
			else {
				// if not writable, then need to write into the app directory instead
				needWriteToAppDir = true;
			}		
		}
		else {
			// if neither XML exists, need to create one of them
			needToCreateFile = true;
			// pick the right one based on writability of cfg directory
			needWriteToAppDir = !pcjHelper::is_dir_writable(gNppMetaInfo.dir.cfgFunctionList);
		}

		// change the destination path if it needs to be in the app directory
		if (needWriteToAppDir) _wsOverMapPath = _wsAppOverMapPath;
	}

	// 2: need XML object
	pOverrideMapXML = new tinyxml2::XMLDocument;		// create new one (old is discarded, if it exists)
	if (needToCreateFile) {
		// create the basic structure
		pRoot = pOverrideMapXML->NewElement("NotepadPlus");
		pOverrideMapXML->InsertFirstChild(pRoot);

		pFunctionList = pRoot->InsertNewChildElement("functionList");
		pAssociationMap = pFunctionList->InsertNewChildElement("associationMap");

		pAssociationMap->InsertEndChild(
			pOverrideMapXML->NewComment(
				"            \n"
				"            See https://github.com/notepad-plus-plus/notepad-plus-plus/blob/master/PowerEditor/installer/functionList/overrideMap.xml for the \"official\" default for this file\n"
				"                tincluding the default id-vs-langID values\n"
				"            \n"
				"            Each functionlist parse rule links to a language ID(\"langID\") or a UDL name.\n"
				"            Examples:\n"
				"                <association id=\"my_perl.xml\" langID=\"21\" />\n"
				"                <association id=\"nppexec.xml\" userDefinedLangName=\"NppExec\" />\n"
				"            "
			)
		);
		pAssociationMap->InsertEndChild(
			pOverrideMapXML->NewComment(" ==================== User Defined Languages ============================ ")
		);
		

		/*tinyxml2::XMLElement* pAssoc =*/ add_udl_assoc("nppexec.xml", "NppExec");
	}
	else {
		// TODO: !!! NEED TO ADD ERROR CHECKING !!!
		// load file and extract the main elements
		/*tinyxml2::XMLError eResult =*/ pOverrideMapXML->LoadFile(sOverMapPath().c_str());
		pRoot = pOverrideMapXML->FirstChildElement("NotepadPlus");
		pFunctionList = pRoot->FirstChildElement("functionList");
		pAssociationMap = pFunctionList->FirstChildElement("associationMap");
	}
}

// add an <association> tag for a given UDL
//		converts wstring to string first
tinyxml2::XMLElement* OverrideMapUpdater::add_udl_assoc(std::wstring wsFilename, std::wstring wsUDLname)
{
	return add_udl_assoc(
		pcjHelper::wstring_to_utf8(wsFilename),
		pcjHelper::wstring_to_utf8(wsUDLname)
	);
}

// add an <association> tag for a given UDL
tinyxml2::XMLElement* OverrideMapUpdater::add_udl_assoc(std::string sFilename, std::string sUDLname)
{
	tinyxml2::XMLElement* pAssoc = pAssociationMap->InsertNewChildElement("association");
	if (pAssoc) {
		pAssoc->SetAttribute("id", sFilename.c_str());
		pAssoc->SetAttribute("userDefinedLangName", sUDLname.c_str());
	}
	return pAssoc;
}

// add <association> tags for each filename,udl pair in the map
std::vector<tinyxml2::XMLElement*> OverrideMapUpdater::add_udl_assoc(std::map<std::string, std::string> msUdls)
{
	std::vector<tinyxml2::XMLElement*> vpAssoc;
	for (const auto& pair : msUdls) {
		vpAssoc.push_back(add_udl_assoc(pair.first, pair.second));
	}
	return vpAssoc;
}

// add <association> tags for each filename,udl pair in the map
std::vector<tinyxml2::XMLElement*> OverrideMapUpdater::add_udl_assoc(std::map<std::wstring, std::wstring> mwsUdls)
{
	std::vector<tinyxml2::XMLElement*> vpAssoc;
	for (const auto& pair : mwsUdls) {
		vpAssoc.push_back(add_udl_assoc(pair.first, pair.second));
	}
	return vpAssoc;
}

bool OverrideMapUpdater::experiment(void)
{
	add_udl_assoc("fake.xml", "FakeUDL");
	std::map<std::wstring, std::wstring> myMap = {
		{L"udl1.xml", L"UDL 1"},
		{L"udl2.xml", L"UDL 2"},
		{L"udl3.xml", L"UDL 3"}
	};
	add_udl_assoc(myMap);
	pOverrideMapXML->SaveFile(sOverMapPath().c_str());

	return true;
}
