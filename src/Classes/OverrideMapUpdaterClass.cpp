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
		// load file and extract the main elements
		tinyxml2::XMLError eResult = pOverrideMapXML->LoadFile(sOverMapPath().c_str());
		if(_xml_check_result(eResult, pOverrideMapXML, wsOverMapPath())) return;

		pRoot = pOverrideMapXML->FirstChildElement("NotepadPlus");
		if (!pRoot) {
			_xml_check_result(tinyxml2::XML_ERROR_PARSING, pOverrideMapXML, wsOverMapPath());
			return;
		}

		pFunctionList = pRoot->FirstChildElement("functionList");
		if (!pFunctionList) {
			_xml_check_result(tinyxml2::XML_ERROR_PARSING, pOverrideMapXML, wsOverMapPath());
			return;
		}

		pAssociationMap = pFunctionList->FirstChildElement("associationMap");
		if (!pAssociationMap) {
			_xml_check_result(tinyxml2::XML_ERROR_PARSING, pOverrideMapXML, wsOverMapPath());
			return;
		}
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
//		(skip if already exists)
tinyxml2::XMLElement* OverrideMapUpdater::add_udl_assoc(std::string sFilename, std::string sUDLname)
{
	// first check if the UDL already exists in the <association> structure
	tinyxml2::XMLElement* pExist = _find_element_with_attribute_value(pAssociationMap, nullptr, "association", "userDefinedLangName", sUDLname, true);
	if (pExist) return pExist;

	// if it doesn't, then add it
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

// private: case-insensitive std::string equality check
bool _string_insensitive_eq(std::string a, std::string b)
{
	std::string a_copy = "";
	std::string b_copy = "";

	// ignore conversion of int to char implicit in the <algorithm>std::transform, which I have no control over
#pragma warning(push)
#pragma warning(disable: 4244)
	for (size_t i = 0; i < a.size(); i++) { a_copy += std::tolower(a[i]); }
	for (size_t i = 0; i < b.size(); i++) { b_copy += std::tolower(b[i]); }
#pragma warning(pop)
	return a_copy == b_copy;
}

// private: case-insensitive std::string less-than check
bool _string_insensitive_lt(std::string a, std::string b)
{
	std::string a_copy = "";
	std::string b_copy = "";

	// ignore conversion of int to char implicit in the <algorithm>std::transform, which I have no control over
#pragma warning(push)
#pragma warning(disable: 4244)
	for (size_t i = 0; i < a.size(); i++) { a_copy += std::tolower(a[i]); }
	for (size_t i = 0; i < b.size(); i++) { b_copy += std::tolower(b[i]); }
#pragma warning(pop)
	return a_copy < b_copy;
}


// look for an element, based on {Parent, FirstChild, or both} which is of a specific ElementType, having a specific AttributeName with specific AttributeValue
tinyxml2::XMLElement* OverrideMapUpdater::_find_element_with_attribute_value(tinyxml2::XMLElement* pParent, tinyxml2::XMLElement* pFirst, std::string sElementType, std::string sAttributeName, std::string sAttributeValue, bool caseSensitive)
{
	if (!pParent && !pFirst) return nullptr;
	tinyxml2::XMLElement* pMyParent = pParent ? pParent->ToElement() : pFirst->Parent()->ToElement();
	tinyxml2::XMLElement* pFCE = pMyParent->FirstChildElement(sElementType.c_str());
	if (!pFirst && !pFCE) return nullptr;
	tinyxml2::XMLElement* pFoundElement = pFirst ? pFirst->ToElement() : pFCE->ToElement();
	while (pFoundElement) {
		// if this node has the right attribute pair, great!
		if (caseSensitive) {
			if (pFoundElement->Attribute(sAttributeName.c_str(), sAttributeValue.c_str()))
				return pFoundElement;
		}
		else {
			const char* cAttrValue = pFoundElement->Attribute(sAttributeName.c_str());
			if (cAttrValue) {
				if (_string_insensitive_eq(sAttributeValue, cAttrValue))
					return pFoundElement;
			}
		}

		// otherwise, move on to next
		pFoundElement = pFoundElement->NextSiblingElement(sElementType.c_str());
	}
	return pFoundElement;
}


// compares the XMLError result to XML_SUCCESS, and returns a TRUE boolean to indicate failure
//		p_doc defaults to nullptr, wsFilePath to L""
bool OverrideMapUpdater::_xml_check_result(tinyxml2::XMLError a_eResult, tinyxml2::XMLDocument* p_doc, std::wstring wsFilePath)
{
	if (a_eResult != tinyxml2::XML_SUCCESS) {
		std::string sMsg = std::string("XML Error #") + std::to_string(static_cast<int>(a_eResult));
		if (p_doc != NULL) {
			sMsg += std::string(": ") + std::string(p_doc->ErrorStr());
			if (p_doc->ErrorLineNum()) {
				sMsg += "\n\nI will try to open the file near that error.";
			}
		}
		::MessageBoxA(NULL, sMsg.c_str(), "XML Error", MB_ICONWARNING | MB_OK);
		if (p_doc != NULL && p_doc->ErrorLineNum() && wsFilePath.size()) {
			if (::SendMessage(gNppMetaInfo.hwnd._nppHandle, NPPM_DOOPEN, 0, reinterpret_cast<LPARAM>(wsFilePath.c_str()))) {
				extern NppData nppData;	// not in PluginDefinition.h

				// Get the current scintilla
				int which = -1;
				::SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&which);
				HWND curScintilla = (which < 1) ? nppData._scintillaMainHandle : nppData._scintillaSecondHandle;

				// SCI_GOTOLINE in the current scintilla instance
				WPARAM zeroLine = static_cast<WPARAM>(p_doc->ErrorLineNum() - 1);
				::SendMessage(curScintilla, SCI_GOTOLINE, zeroLine, 0);

				// do annotation
				::SendMessage(curScintilla, SCI_ANNOTATIONCLEARALL, 0, 0);
				::SendMessage(curScintilla, SCI_ANNOTATIONSETVISIBLE, ANNOTATION_BOXED, 0);
				::SendMessageA(curScintilla, SCI_ANNOTATIONSETTEXT, zeroLine, reinterpret_cast<LPARAM>(p_doc->ErrorStr()));

				// need to stop
				// In ConfigUpdater, I needed to set `_doAbort = true;` to exit out of a loop, but that's needed in CollectionInterface
			};
		}
		return true;
	}
	return false;
};
