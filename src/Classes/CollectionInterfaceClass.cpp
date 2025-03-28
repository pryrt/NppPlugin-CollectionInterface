#include "CollectionInterfaceClass.h"
// nlohmann/json.hpp
#include "json.hpp"

CollectionInterface::CollectionInterface(void) {
	vwsUDLFiles.push_back(L"udl1.xml");
	vwsUDLFiles.push_back(L"udl2.xml");
	vwsUDLFiles.push_back(L"udl3.xml");
	vwsACFiles.push_back(L"ac1.xml");
	vwsACFiles.push_back(L"ac2.xml");
	vwsFLFiles.push_back(L"fl1.xml");
	vwsFLFiles.push_back(L"fl2.xml");
	vwsThemeFiles.push_back(L"themeA");
};

std::vector<char> CollectionInterface::downloadFileInMemory(const std::string& url)
{
#if 1
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



#else
	std::vector<char> buffer;
	HINTERNET hInternet = nullptr;
	HINTERNET hConnect = nullptr;
	HINTERNET hRequest = nullptr;

	try {
		hInternet = InternetOpenA("HTTPDownloader", INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, 0);
		if (!hInternet) {
			throw std::runtime_error("InternetOpen failed");
		}

		URL_COMPONENTSA urlComp;
		memset(&urlComp, 0, sizeof(urlComp));
		urlComp.dwStructSize = sizeof(urlComp);
		urlComp.dwSchemeLength = static_cast<DWORD>(-1);
		urlComp.dwHostNameLength = static_cast<DWORD>(-1);
		urlComp.dwUrlPathLength = static_cast<DWORD>(-1);

		if (!InternetCrackUrlA(url.c_str(), 0, 0, &urlComp))
		{
			throw std::runtime_error("InternetCrackUrl failed");
		}

		hConnect = InternetConnectA(hInternet, urlComp.lpszHostName, urlComp.nPort, nullptr, nullptr, INTERNET_SERVICE_HTTP, 0, 0);
		if (!hConnect) {
			throw std::runtime_error("InternetConnect failed");
		}

		hRequest = HttpOpenRequestA(hConnect, "GET", urlComp.lpszUrlPath, nullptr, nullptr, nullptr, 0, 0);
		if (!hRequest) {
			throw std::runtime_error("HttpOpenRequest failed");
		}

		if (!HttpSendRequest(hRequest, nullptr, 0, nullptr, 0)) {
			throw std::runtime_error("HttpSendRequest failed");
		}

		DWORD bytesRead;
		char tempBuffer[1024];
		while (InternetReadFile(hRequest, tempBuffer, sizeof(tempBuffer), &bytesRead) && bytesRead > 0) {
			buffer.insert(buffer.end(), tempBuffer, tempBuffer + bytesRead);
		}

		if (GetLastError() != ERROR_SUCCESS && GetLastError() != ERROR_IO_PENDING && GetLastError() != ERROR_MORE_DATA && bytesRead == 0)
		{
			throw std::runtime_error("InternetReadFile failed");
		}
	}
	catch (const std::exception& e) {
		std::string sMsg;
		sMsg += "Error downloading " + url + "\n>> " + e.what();
		int sz = MultiByteToWideChar(CP_UTF8, 0, sMsg.c_str(), -1, nullptr, 0);
		std::wstring wsMsg(sz, 0);
		MultiByteToWideChar(CP_UTF8, 0, sMsg.c_str(), -1, (wchar_t*)wsMsg.c_str(), sz);
		::MessageBox(NULL, wsMsg.c_str(), L"Download Error", MB_ICONERROR);
		buffer.clear();
	}
	if (hRequest) InternetCloseHandle(hRequest);
	if (hConnect) InternetCloseHandle(hConnect);
	if (hInternet) InternetCloseHandle(hInternet);

	return buffer;
#endif
}

void CollectionInterface::getListsFromJson(void)
{
	std::vector<char> vcThemeJSON = downloadFileInMemory("https://raw.githubusercontent.com/notepad-plus-plus/nppThemes/master/themes/.toc.json");
	nlohmann::json jTheme = nlohmann::json::parse(vcThemeJSON);
	std::string v = jTheme.at(0).get<std::string>();
	//::MessageBoxA(NULL, v.c_str(), "DebugJson", MB_OK);
	for (const auto& item : jTheme.items()) {
		::MessageBoxA(NULL, item.value().get<std::string>().c_str(), "IterateThemes", MB_OK);
	}

	std::vector<char> vcUdlJSON = downloadFileInMemory("https://raw.githubusercontent.com/notepad-plus-plus/userDefinedLanguages/refs/heads/master/udl-list.json");
	nlohmann::json jUdl = nlohmann::json::parse(vcUdlJSON);
	// iterate over all the items (key/value pairs) in the top=level, where key() is the name of the key and value() is the object the entry contains
	for (const auto& item : jUdl.items()) {
		::MessageBoxA(NULL, item.key().c_str(), "IterateUdlKeys", MB_OK);
	}
	// iterate over all the elements of the "UDLs" list; for a list, the key() is just the index, and the value() is the sub-object
	for (const auto& item : jUdl["UDLs"].items()) {
		auto j = item.value();
		//::MessageBoxA(NULL, item.value()["id-name"].get<std::string>().c_str(), "IterateUdlList[id-name]", MB_OK);
		if (j.contains("functionList")) {
			::MessageBoxA(NULL, item.value()["id-name"].get<std::string>().c_str(), "IterateUdlList[id-name]: has functionList", MB_OK);
			::MessageBoxA(NULL, j["functionList"].get<std::string>().c_str(), "Entry with functionList", MB_OK);
		}
		if (j.contains("autoCompletion")) {
			::MessageBoxA(NULL, item.value()["id-name"].get<std::string>().c_str(), "IterateUdlList[id-name]: has autoCompletion", MB_OK);
			//::MessageBoxA(NULL, j["autoCompletion"].get<std::string>().c_str(), "Entry with autoCompletion", MB_OK);
			// TODO: cannot yet handle boolean (true/false) vs string, which I'll need to be able to handle
		}
	}

	return;
}
