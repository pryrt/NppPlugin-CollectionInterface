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
			pOverrideMapXML->NewComment("\r\n"
				"\t\t\tSee https://github.com/notepad-plus-plus/notepad-plus-plus/blob/master/PowerEditor/installer/functionList/overrideMap.xml for the \"official\" default for this file\r\n"
				"\t\t\t\tincluding the default id-vs-langID values\r\n"
				"\r\n"
				"\t\t\tEach functionlist parse rule links to a language ID(\"langID\") or a UDL name.\r\n"
				"\t\t\tExamples:\r\n"
				"\t\t\t\t<association id=\"my_perl.xml\" langID=\"21\" />\r\n"
				"\t\t\t\t<association id=\"nppexec.xml\" userDefinedLangName=\"NppExec\" />\r\n"
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

bool OverrideMapUpdater::experiment(void)
{
	add_udl_assoc("fake.xml", "FakeUDL");
	pOverrideMapXML->SaveFile(sOverMapPath().c_str());
	return true;
}
