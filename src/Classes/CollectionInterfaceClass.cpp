#include "CollectionInterfaceClass.h"
// nlohmann/json.hpp
#include "json.hpp"

CollectionInterface::CollectionInterface(void) {
	getListsFromJson();
};

std::vector<char> CollectionInterface::downloadFileInMemory(const std::string& url)
{
	HINTERNET hInternet = InternetOpenA("MyUserAgent", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
	if (hInternet == NULL) {
		throw std::runtime_error("InternetOpen failed");
	}

	HINTERNET hConnect = InternetOpenUrlA(hInternet, url.c_str(), NULL, 0, INTERNET_FLAG_RELOAD, 0);
	if (hConnect == NULL) {
		std::string errmsg = "InternetOpenUrl failed: " + std::to_string(GetLastError()) + "\n";
		throw std::runtime_error(errmsg.c_str());
	}

	std::vector<char> buffer(4096);
	std::vector<char> response_data;
	DWORD bytes_read;

	while (InternetReadFile(hConnect, buffer.data(), static_cast<DWORD>(buffer.size()), &bytes_read) && bytes_read > 0) {
		response_data.insert(response_data.end(), buffer.begin(), buffer.begin() + bytes_read);
	}

	if(hConnect) InternetCloseHandle(hConnect);
	if(hInternet) InternetCloseHandle(hInternet);

	return response_data;
}

#pragma warning ( push )
#pragma warning ( disable: 4101 )
// from google AI's answer to "c++ equivalent of python html.unescape"
//	it only does the basic 5 XML entities, or numeric/hex entities; other named entities are essentially left alone
std::string CollectionInterface::_xml_unentity(const std::string& text)
{
	std::string result;
	size_t i = 0;
	while (i < text.size()) {
		if (text[i] == '&') {
			size_t pos = text.find(';', i);
			if (pos != std::string::npos) {
				std::string entity = text.substr(i + 1, pos - i - 1);
				if (entity == "amp") result += '&';
				else if (entity == "lt") result += '<';
				else if (entity == "gt") result += '>';
				else if (entity == "quot") result += '"';
				else if (entity == "apos") result += '\'';
				else if (entity.substr(0, 2) == "#x") {
					try {
						result += static_cast<char>(std::stoi(entity.substr(2), nullptr, 16));
					}
					catch (const std::exception& e) {
						result += '&' + entity + ';';
					}
				}
				else if (entity[0] == '#') {
					try {
						result += static_cast<char>(std::stoi(entity.substr(1)));
					}
					catch (const std::exception& e) {
						result += '&' + entity + ';';
					}
				}
				else {
					result += '&' + entity + ';';
				}
				i = pos + 1;
				continue;
			}
		}
		result += text[i];
		i++;
	}
	return result;

}
#pragma warning ( pop )

void CollectionInterface::getListsFromJson(void)
{
	////////////////////////////////
	// Process Theme JSON
	////////////////////////////////
	// TODO:
	std::vector<char> vcThemeJSON = downloadFileInMemory("https://raw.githubusercontent.com/notepad-plus-plus/nppThemes/master/themes/.toc.json");
	nlohmann::json jTheme = nlohmann::json::parse(vcThemeJSON);
	std::string v = jTheme.at(0).get<std::string>();
	for (const auto& item : jTheme.items()) {
		vThemeFiles.push_back(item.value().get<std::string>().c_str());
		//!!	::MessageBoxA(NULL, item.value().get<std::string>().c_str(), "IterateThemes", MB_OK);
	}

	////////////////////////////////
	// Process UDL JSON
	////////////////////////////////
	std::vector<char> vcUdlJSON = downloadFileInMemory("https://raw.githubusercontent.com/notepad-plus-plus/userDefinedLanguages/refs/heads/master/udl-list.json");
	nlohmann::json jUdl = nlohmann::json::parse(vcUdlJSON);
	// for a list, the key() is just the index, and the value() is the sub-object
	for (const auto& item : jUdl["UDLs"].items()) {
		auto j = item.value();
		std::string id_name = j["id-name"].get<std::string>();
		std::string udl_base = "https://raw.githubusercontent.com/notepad-plus-plus/userDefinedLanguages/master/";

		// Logic for UDL -> URL
		if (j.contains("repository")) {
			std::string sUDL = "";
			if (j["repository"].is_boolean()) {		// URL repo should never be boolean; but if it is, generate default URL
				sUDL = udl_base + "UDLs/" + id_name + ".xml";
			}
			if (j["repository"].is_string()) {
				std::string s = j["repository"].get<std::string>();
				if (s == "") {
					sUDL = udl_base + "UDLs/" + id_name + ".xml";
				}
				else if (s.find("http") == 0) {	// if string _starts_ with http or https, it's the full URL
					sUDL = s;
				}
			}

			// assign into the data structure...
			if (sUDL != "") {
				mapUDL[id_name] = sUDL;
			}
		}

		// Extract display-name
		if (j.contains("display-name")) {
			std::string display_name = _xml_unentity(j["display-name"].get<std::string>());

			if (display_name != j["display-name"].get<std::string>()) {
				std::string msg = "Convert\n" + j["display-name"].get<std::string>() + "\ninto\n" + display_name;
			}

			// assign into the data structure...
			if (display_name != "") {
				mapDISPLAY[id_name] = display_name;
				revDISPLAY[display_name] = id_name;
			}
		}
		
		// Logic for functionList -> URL
		if (j.contains("functionList")) {
			std::string sFuncList = "";
			if (j["functionList"].is_boolean() && j["functionList"].get<bool>()) {
				sFuncList = udl_base + "functionList/" + id_name + ".xml";
			}
			if (j["functionList"].is_string()) {
				std::string s = j["functionList"].get<std::string>();
				if (s.find("http") == 0) {	// if string _starts_ with http or https, it's the full URL
					sFuncList = s;
				}
				else {
					sFuncList = udl_base + "functionList/" + s + ".xml";
				}
			}

			// assign sFuncList into the data structure...
			if (sFuncList != "") {
				mapFL[id_name] = sFuncList;
			}
		}

		// Logic for autoCompletion -> URL
		if (j.contains("autoCompletion")) {
			std::string sAutoComp = "";
			1;//::MessageBoxA(NULL, item.value()["id-name"].get<std::string>().c_str(), "IterateUdlList[id-name]: has autoCompletion", MB_OK);
			if (j["autoCompletion"].is_boolean()) {
				sAutoComp = udl_base + "autoCompletion/" + id_name + ".xml";
			}
			if (j["autoCompletion"].is_string()) {
				auto s = j["autoCompletion"].get<std::string>();
				if (s.find("http") == 0) {
					sAutoComp = s;
				}
				else {
					sAutoComp = udl_base + "autoCompletion/" + s + ".xml";
				}
			}

			// assign sAutoComp into the data structure...
			if (sAutoComp != "") {
				mapAC[id_name] = sAutoComp;
			}
		}
	}

	return;
}
