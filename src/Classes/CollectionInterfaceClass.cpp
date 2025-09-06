#include "CollectionInterfaceClass.h"
// nlohmann/json.hpp
#include "json.hpp"
#include <Shlwapi.h>

// private
void delNull(std::wstring& str)
{
	const auto pos = str.find(L'\0');
	if (pos != std::wstring::npos) {
		str.erase(pos);
	}
}

CollectionInterface::CollectionInterface(HWND /*hwndNpp*/) {
	gNppMetaInfo.populate();
	_areListsPopulated = getListsFromJson();
};

std::vector<char> CollectionInterface::downloadFileInMemory(const std::wstring& url)
{
	HINTERNET hInternet = InternetOpen(L"CollectionInterfacePluginForN++", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
	if (hInternet == NULL) {
		std::wstring errmsg = L"Could not connect to internet when trying to download\r\n" + url;
		::MessageBox(gNppMetaInfo.hwnd._nppHandle, errmsg.c_str(), L"Download Error", MB_ICONERROR);
		return std::vector<char>();
	}

	HINTERNET hConnect = InternetOpenUrl(hInternet, url.c_str(), NULL, 0, INTERNET_FLAG_RELOAD, 0);
	if (hConnect == NULL) {
		std::wstring errmsg = L"Could not connect to internet when trying to download\r\n" + url;
		::MessageBox(gNppMetaInfo.hwnd._nppHandle, errmsg.c_str(), L"Download Error", MB_ICONERROR);
		return std::vector<char>();
	}

	std::vector<char> buffer(4096);
	std::vector<char> response_data;
	DWORD bytes_read;

	while (InternetReadFile(hConnect, buffer.data(), static_cast<DWORD>(buffer.size()), &bytes_read) && bytes_read > 0) {
		response_data.insert(response_data.end(), buffer.begin(), buffer.begin() + bytes_read);
	}

	if (hConnect) InternetCloseHandle(hConnect);
	if (hInternet) InternetCloseHandle(hInternet);

	return response_data;
}

bool CollectionInterface::downloadFileToDisk(const std::wstring& url, const std::wstring& path)
{
	// first expand any variables in the the path
	DWORD dwExSize = 2 * static_cast<DWORD>(path.size());
	std::wstring expandedPath(dwExSize, L'\0');
	if (!ExpandEnvironmentStrings(path.c_str(), const_cast<LPWSTR>(expandedPath.data()), dwExSize)) {
		std::wstring errmsg = L"Could not understand the path \"" + path + L"\": " + std::to_wstring(GetLastError()) + L"\n";
		::MessageBox(gNppMetaInfo.hwnd._nppHandle, errmsg.c_str(), L"Download Error", MB_ICONERROR);
		return false;
	}
	pcjHelper::delNull(expandedPath);

	// don't download and overwrite the file if it already exists
	if (!ask_overwrite_if_exists(expandedPath)) {
		return false;
	}

	// now that I know it's safe to write the file, try opening the internet connection
	HINTERNET hInternet = InternetOpen(L"CollectionInterfacePluginForN++", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
	if (hInternet == NULL) {
		std::wstring errmsg = L"Could not connect to internet when trying to download\r\n" + url;
		::MessageBox(gNppMetaInfo.hwnd._nppHandle, errmsg.c_str(), L"Download Error", MB_ICONERROR);
		return false;
	}

	HINTERNET hConnect = InternetOpenUrl(hInternet, url.c_str(), NULL, 0, INTERNET_FLAG_RELOAD, 0);
	if (hConnect == NULL) {
		std::wstring errmsg = L"Could not connect to internet when trying to download\r\n" + url;
		::MessageBox(gNppMetaInfo.hwnd._nppHandle, errmsg.c_str(), L"Download Error", MB_ICONERROR);
		if (hInternet) InternetCloseHandle(hInternet);	// need to free hInternet if hConnect failed
		return false;
	}

	HANDLE hFile = CreateFile(expandedPath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		DWORD errNum = GetLastError();
		LPWSTR messageBuffer = nullptr;
		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			errNum,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPWSTR)&messageBuffer,
			0,
			NULL
		);

		std::wstring errmsg = L"Could not create the file \"" + expandedPath + L"\": " + std::to_wstring(GetLastError()) + L" => " + messageBuffer + L"\n";
		::MessageBox(NULL, errmsg.c_str(), L"Download Error", MB_ICONERROR);

		// need to free hInternet, hConnect, and messageBuffer
		if (hConnect) InternetCloseHandle(hConnect);
		if (hConnect) InternetCloseHandle(hConnect);
		LocalFree(messageBuffer);
		return false;
	}

	std::vector<char> buffer(4096);
	for (DWORD dwBytesRd = 1; dwBytesRd > 0; ) {
		// read a chunk from the webfile
		BOOL stat = InternetReadFile(hConnect, buffer.data(), static_cast<DWORD>(buffer.size()), &dwBytesRd);
		if (!stat) {
			std::wstring errmsg = L"Error reading from URL\"" + url + L"\": " + std::to_wstring(GetLastError()) + L"\n";
			::MessageBox(NULL, errmsg.c_str(), L"Download Error", MB_ICONERROR);
			// free handles before returning
			if (hConnect) InternetCloseHandle(hConnect);
			if (hInternet) InternetCloseHandle(hInternet);
			if (hFile) CloseHandle(hFile);
			return false;
		}

		// don't need to write if no bytes were read (ie, EOF)
		if (!dwBytesRd) break;

		// write the chunk to the path
		DWORD dwBytesWr = 0;
		stat = WriteFile(hFile, buffer.data(), dwBytesRd, &dwBytesWr, NULL);
		if (!stat) {
			std::wstring errmsg = L"Error writing to \"" + expandedPath + L"\": " + std::to_wstring(GetLastError()) + L"\n";
			::MessageBox(NULL, errmsg.c_str(), L"Download Error", MB_ICONERROR);
			// free handles before returning
			if (hConnect) InternetCloseHandle(hConnect);
			if (hInternet) InternetCloseHandle(hInternet);
			if (hFile) CloseHandle(hFile);
			return false;
		}
	}

	if (hConnect) InternetCloseHandle(hConnect);
	if (hInternet) InternetCloseHandle(hInternet);
	if (hFile) CloseHandle(hFile);

	return true;
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

bool CollectionInterface::getListsFromJson(void)
{
	bool didThemeFail = false;
	auto string2wstring = [](std::string str) {
		if (str.empty()) return std::wstring();
		int wsz = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(str.size()), NULL, 0);
		std::wstring ret(wsz, 0);
		MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(str.size()), const_cast<LPWSTR>(ret.data()), wsz);
		return ret;
		};

	////////////////////////////////
	// Process Theme JSON
	////////////////////////////////
	std::vector<char> vcThemeJSON = downloadFileInMemory(L"https://raw.githubusercontent.com/notepad-plus-plus/nppThemes/master/themes/.toc.json");
	if (vcThemeJSON.empty()) {
		// issue#13: do not continue if there's internet/connection problems
		return false;	// nothing downloaded, so want to know to close the download-dialog to avoid annoying user with useless empty listbox
	}
	else if (vcThemeJSON[0] != L'[') {
		// related to issue#13: if downloadFileInMemory returns "404 Not Found" or similar, don't try to parse as JSON.
		//	easiest check: if the JSON isn't the expected [...] JSON array, don't continue with the _theme_;
		//			however, can still move to the UDL section, because that might still work, and since UDL is the primary purpose, it's probably worth it if UDL is working even if Themes aren't.
		std::string msg = "Cannot interpret Themes Collection information:\n\n";
		msg += vcThemeJSON.data();
		if (msg.size() > 100) {
			msg.resize(100);
			msg += "\n...";
		}
		::MessageBoxA(gNppMetaInfo.hwnd._nppHandle, msg.c_str(), "CollectionInterface: Download Problems", MB_ICONWARNING);
		didThemeFail = true;
	}
	else {
		try
		{
			nlohmann::json jTheme = nlohmann::json::parse(vcThemeJSON);
			std::string v = jTheme.at(0).get<std::string>();
			for (const auto& item : jTheme.items()) {
				std::wstring ws = string2wstring(item.value().get<std::string>());
				vThemeFiles.push_back(ws.c_str());
			}
		}
		catch (nlohmann::json::exception& e) {
			std::string msg = std::string("JSON Error in Theme data: ") + e.what();
			::MessageBoxA(gNppMetaInfo.hwnd._nppHandle, msg.c_str(), "CollectionInterface: JSON Error", MB_ICONERROR);
			didThemeFail = true;
		}
		catch (std::exception& e) {
			std::string msg = std::string("Unrecognized Error in Theme data: ") + e.what();
			::MessageBoxA(gNppMetaInfo.hwnd._nppHandle, msg.c_str(), "CollectionInterface: Unrecognized Error", MB_ICONERROR);
			didThemeFail = true;
		}
	}

	////////////////////////////////
	// Process UDL JSON
	////////////////////////////////
	std::vector<char> vcUdlJSON = downloadFileInMemory(L"https://raw.githubusercontent.com/notepad-plus-plus/userDefinedLanguages/refs/heads/master/udl-list.json");
	if (vcUdlJSON.empty()) {
		// issue#13: do not continue if there's internet/connection problems
		return false;	// nothing downloaded, so want to know to close the download-dialog to avoid annoying user with useless empty listbox
	}
	else if (vcUdlJSON[0] != L'{') {
		// related to issue#13: if downloadFileInMemory returns "404 Not Found" or similar, don't try to parse as JSON.
		//	easiest check: if the JSON isn't the expected [...] JSON array, don't continue with the _theme_;
		std::string msg = "Cannot interpret UDL Collection information:\n\n";
		msg += vcUdlJSON.data();
		if (msg.size() > 100) {
			msg.resize(100);
			msg += "\n...";
		}
		if (!didThemeFail)
			::MessageBoxA(gNppMetaInfo.hwnd._nppHandle, msg.c_str(), "CollectionInterface: Download Problems", MB_ICONWARNING);
		return false;	// without UDL info, it's not worth displaying the Download Dialog
	}
	else {
		try
		{
			nlohmann::json jUdl = nlohmann::json::parse(vcUdlJSON);
			// for a list, the key() is just the index, and the value() is the sub-object
			for (const auto& item : jUdl["UDLs"].items()) {
				auto j = item.value();
				std::wstring ws_id_name = string2wstring(j["id-name"].get<std::string>());
				std::wstring udl_base = L"https://raw.githubusercontent.com/notepad-plus-plus/userDefinedLanguages/master/";

				// Logic for UDL -> URL
				if (j.contains("repository")) {
					std::wstring sUDL = L"";
					if (j["repository"].is_boolean()) {		// URL repo should never be boolean; but if it is, generate default URL
						sUDL = udl_base + L"UDLs/" + ws_id_name + L".xml";
					}
					if (j["repository"].is_string()) {
						std::wstring ws = string2wstring(j["repository"].get<std::string>());
						if (ws == L"") {
							sUDL = udl_base + L"UDLs/" + ws_id_name + L".xml";
						}
						else if (ws.find(L"http") == 0) {	// if string _starts_ with http or https, it's the full URL
							sUDL = ws;
						}
					}

					// assign into the data structure...
					if (sUDL != L"") {
						mapUDL[ws_id_name] = sUDL;
					}
				}

				// Extract display-name
				if (j.contains("display-name")) {
					std::wstring wdisplay_name = string2wstring(_xml_unentity(j["display-name"].get<std::string>()));

					// assign into the data structure...
					if (wdisplay_name != L"") {
						mapDISPLAY[ws_id_name] = wdisplay_name;
						revDISPLAY[wdisplay_name] = ws_id_name;
					}
				}

				// Logic for functionList -> URL
				if (j.contains("functionList")) {
					std::wstring wsFuncList = L"";
					if (j["functionList"].is_boolean() && j["functionList"].get<bool>()) {
						wsFuncList = udl_base + L"functionList/" + ws_id_name + L".xml";
					}
					if (j["functionList"].is_string()) {
						std::wstring ws = string2wstring(j["functionList"].get<std::string>());

						if (ws.find(L"http") == 0) {	// if string _starts_ with http or https, it's the full URL
							wsFuncList = ws;
						}
						else {
							wsFuncList = udl_base + L"functionList/" + ws + L".xml";
						}
					}

					// assign wsFuncList into the data structure...
					if (wsFuncList != L"") {
						mapFL[ws_id_name] = wsFuncList;
					}
				}

				// Logic for autoCompletion -> URL
				if (j.contains("autoCompletion")) {
					std::wstring wsAutoComp = L"";
					if (j["autoCompletion"].is_boolean()) {
						wsAutoComp = udl_base + L"autoCompletion/" + ws_id_name + L".xml";
					}
					if (j["autoCompletion"].is_string()) {
						std::wstring ws = string2wstring(j["autoCompletion"].get<std::string>());
						if (ws.find(L"http") == 0) {
							wsAutoComp = ws;
						}
						else {
							wsAutoComp = udl_base + L"autoCompletion/" + ws + L".xml";
						}
					}

					// assign sAutoComp into the data structure...
					if (wsAutoComp != L"") {
						mapAC[ws_id_name] = wsAutoComp;
					}
				}
			}
		}
		catch (nlohmann::json::exception& e) {
			std::string msg = std::string("JSON Error in UDL data: ") + e.what();
			::MessageBoxA(gNppMetaInfo.hwnd._nppHandle, msg.c_str(), "CollectionInterface: JSON Error", MB_ICONERROR);
			return false;	// without UDL info, it's not worth displaying the Download Dialog
		}
		catch (std::exception& e) {
			std::string msg = std::string("Unrecognized Error in UDL data: ") + e.what();
			::MessageBoxA(gNppMetaInfo.hwnd._nppHandle, msg.c_str(), "CollectionInterface: Unrecognized Error", MB_ICONERROR);
			return false;	// without UDL info, it's not worth displaying the Download Dialog
		}
	}

	// if it makes it here, there is enough data to be worth displaying the dialog
	return true;
}

std::wstring CollectionInterface::getWritableTempDir(void)
{
	// first try the system TEMP
	std::wstring tempDir(MAX_PATH + 1, L'\0');
	GetTempPath(MAX_PATH + 1, const_cast<LPWSTR>(tempDir.data()));
	pcjHelper::delNull(tempDir);

	// if that fails, try c:\tmp or c:\temp
	if (!pcjHelper::is_dir_writable(tempDir)) {
		tempDir = L"c:\\temp";
		pcjHelper::delNull(tempDir);
	}
	if (!pcjHelper::is_dir_writable(tempDir)) {
		tempDir = L"c:\\tmp";
		pcjHelper::delNull(tempDir);
	}

	// if that fails, try the %USERPROFILE%
	if (!pcjHelper::is_dir_writable(tempDir)) {
		tempDir.resize(MAX_PATH + 1);
		if (!ExpandEnvironmentStrings(L"%USERPROFILE%", const_cast<LPWSTR>(tempDir.data()), MAX_PATH + 1)) {
			std::wstring errmsg = L"getWritableTempDir::ExpandEnvirontmentStrings(%USERPROFILE%) failed: " + std::to_wstring(GetLastError()) + L"\n";
			::MessageBox(NULL, errmsg.c_str(), L"Directory Error", MB_ICONERROR);
			return L"";
		}
		pcjHelper::delNull(tempDir);
	}

	// last try: current directory
	if (!pcjHelper::is_dir_writable(tempDir)) {
		tempDir.resize(MAX_PATH + 1);
		GetCurrentDirectory(MAX_PATH + 1, const_cast<LPWSTR>(tempDir.data()));
		pcjHelper::delNull(tempDir);
	}

	// if that fails, no other ideas
	if (!pcjHelper::is_dir_writable(tempDir)) {
		std::wstring errmsg = L"getWritableTempDir() cannot find any writable directory; please make sure %TEMP% is defined and writable\n";
		::MessageBox(NULL, errmsg.c_str(), L"Directory Error", MB_ICONERROR);
		return L"";
	}
	return tempDir;
}

bool CollectionInterface::ask_overwrite_if_exists(const std::wstring& path)
{
	if (!PathFileExists(path.c_str())) return true;	// if file doesn't exist, it's okay to "overwrite" nothing ;-)
	std::wstring msg = L"The path\r\n" + path + L"\r\nalready exists.  Should I overwrite it?";
	int ans = ::MessageBox(gNppMetaInfo.hwnd._nppHandle, msg.c_str(), L"Overwrite File?", MB_YESNO);
	return ans == IDYES;
}
