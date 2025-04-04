#include "CollectionInterfaceClass.h"
// nlohmann/json.hpp
#include "json.hpp"


class wruntime_error : public std::runtime_error {
public:
	wruntime_error(const std::wstring& msg) : runtime_error("Error!"), message(msg) {};

	std::wstring get_message() { return message; }

private:
	std::wstring message;
};

CollectionInterface::CollectionInterface(HWND hwndNpp) {
	_hwndNPP = hwndNpp;
	_populateNppDirs();
	getListsFromJson();
};

void CollectionInterface::_populateNppDirs(void) {
	auto delNull = [](std::wstring& str) {
		const auto pos = str.find(L'\0');
		if (pos != std::wstring::npos) {
			str.erase(pos);
		}
		};

	// %AppData%\Notepad++\Plugins\Config or equiv
	LRESULT sz = 1 + ::SendMessage(_hwndNPP, NPPM_GETPLUGINSCONFIGDIR, 0, NULL);
	std::wstring pluginCfgDir(sz, '\0');
	::SendMessage(_hwndNPP, NPPM_GETPLUGINSCONFIGDIR, sz, reinterpret_cast<LPARAM>(pluginCfgDir.data()));
	delNull(pluginCfgDir);

	// %AppData%\Notepad++\Plugins or equiv
	//		since it's removing the tail, it will never be longer than pluginCfgDir; since it's in-place, initialize with the first
	std::wstring pluginDir = pluginCfgDir;
	PathCchRemoveFileSpec(const_cast<PWSTR>(pluginDir.data()), pluginCfgDir.size());
	delNull(pluginDir);

	// %AppData%\Notepad++ or equiv is what I'm really looking for
	// _nppCfgDir				#py# _nppConfigDirectory = os.path.dirname(os.path.dirname(notepad.getPluginConfigDir()))
	_nppCfgDir = pluginDir;
	PathCchRemoveFileSpec(const_cast<PWSTR>(_nppCfgDir.data()), pluginDir.size());
	delNull(_nppCfgDir);

	// _nppCfgUdlDir			#py# _nppCfgUdlDirectory = os.path.join(_nppConfigDirectory, 'userDefineLangs')
	_nppCfgUdlDir = _nppCfgDir + L"\\userDefineLangs";

	// _nppCfgFunctionListDir	#py# _nppCfgFunctionListDirectory = os.path.join(_nppConfigDirectory, 'functionList')
	_nppCfgFunctionListDir = _nppCfgDir + L"\\functionList";

	// _nppCfgThemesDir			#py# _nppCfgThemesDirectory = os.path.join(_nppConfigDirectory, 'themes')
	_nppCfgThemesDir = _nppCfgDir + L"\\themes";

	// AutoCompletion is _always_ relative to notepad++.exe, never in AppData or CloudConfig or SettingsDir
	// _nppCfgAutoCompletionDir	#py# _nppAppAutoCompletionDirectory = os.path.join(notepad.getNppDir(), 'autoCompletion')
	std::wstring exeDir(MAX_PATH, '\0');
	::SendMessage(_hwndNPP, NPPM_GETNPPDIRECTORY, MAX_PATH, reinterpret_cast<LPARAM>(exeDir.data()));
	delNull(exeDir);
	_nppCfgAutoCompletionDir = exeDir + L"\\autoCompletion";

	return;
}

std::vector<char> CollectionInterface::downloadFileInMemory(const std::wstring& url)
{
	HINTERNET hInternet = InternetOpen(L"CollectionInterfacePluginForN++", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
	if (hInternet == NULL) {
		throw std::runtime_error("InternetOpen failed");
	}

	HINTERNET hConnect = InternetOpenUrl(hInternet, url.c_str(), NULL, 0, INTERNET_FLAG_RELOAD, 0);
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

	if (hConnect) InternetCloseHandle(hConnect);
	if (hInternet) InternetCloseHandle(hInternet);

	return response_data;
}

bool CollectionInterface::downloadFileToDisk(const std::wstring& url, const std::wstring& path)
{
	HINTERNET hInternet = InternetOpen(L"CollectionInterfacePluginForN++", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
	if (hInternet == NULL) {
		throw wruntime_error(L"InternetOpen failed");
	}

	HINTERNET hConnect = InternetOpenUrl(hInternet, url.c_str(), NULL, 0, INTERNET_FLAG_RELOAD, 0);
	if (hConnect == NULL) {
		std::string errmsg = "InternetOpenUrl failed: " + std::to_string(GetLastError()) + "\n";
		throw std::runtime_error(errmsg.c_str());
	}

	DWORD dwExSize = 2 * static_cast<DWORD>(path.size());
	std::wstring expandedPath(dwExSize, L'\0');
	if (!ExpandEnvironmentStrings(path.c_str(), const_cast<LPWSTR>(expandedPath.data()), dwExSize)) {
		std::wstring errmsg = L"ExpandEnvirontmentStrings(" + path + L") failed: " + std::to_wstring(GetLastError()) + L"\n";
		throw wruntime_error(errmsg.c_str());
	}
	_wsDeleteTrailingNulls(expandedPath);

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

		std::wstring errmsg = L"CreateFile(" + expandedPath + L") failed: " + std::to_wstring(GetLastError()) + L" => " + messageBuffer + L"\n";
		::MessageBox(NULL, errmsg.c_str(), L"Command Error", MB_ICONERROR);

		LocalFree(messageBuffer);

		throw wruntime_error(errmsg.c_str());
	}

	std::vector<char> buffer(4096);
	for (DWORD dwBytesRd = 1; dwBytesRd > 0; ) {
		// read a chunk from the webfile
		BOOL stat = InternetReadFile(hConnect, buffer.data(), static_cast<DWORD>(buffer.size()), &dwBytesRd);
		if (!stat) {
			std::wstring errmsg = L"InternetReadFile(" + url + L") failed: " + std::to_wstring(GetLastError()) + L"\n";
			throw wruntime_error(errmsg.c_str());
		}

		// don't need to write if no bytes were read (ie, EOF)
		if (!dwBytesRd) break;

		// write the chunk to the path
		DWORD dwBytesWr = 0;
		stat = WriteFile(hFile, buffer.data(), dwBytesRd, &dwBytesWr, NULL);
		if (!stat) {
			std::wstring errmsg = L"WriteFile(" + expandedPath + L") failed: " + std::to_wstring(GetLastError()) + L"\n";
			throw wruntime_error(errmsg.c_str());
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

void CollectionInterface::getListsFromJson(void)
{
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
	// TODO:
	std::vector<char> vcThemeJSON = downloadFileInMemory(L"https://raw.githubusercontent.com/notepad-plus-plus/nppThemes/master/themes/.toc.json");
	nlohmann::json jTheme = nlohmann::json::parse(vcThemeJSON);
	std::string v = jTheme.at(0).get<std::string>();
	for (const auto& item : jTheme.items()) {
		std::wstring ws = string2wstring(item.value().get<std::string>());
		vThemeFiles.push_back(ws.c_str());
		//!!	::MessageBoxA(NULL, item.value().get<std::string>().c_str(), "IterateThemes", MB_OK);
	}

	////////////////////////////////
	// Process UDL JSON
	////////////////////////////////
	std::vector<char> vcUdlJSON = downloadFileInMemory(L"https://raw.githubusercontent.com/notepad-plus-plus/userDefinedLanguages/refs/heads/master/udl-list.json");
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

	return;
}

std::wstring& CollectionInterface::_wsDeleteTrailingNulls(std::wstring& str)
{
	const auto pos = str.find(L'\0');
	if (pos != std::wstring::npos) {
		str.erase(pos);
	}
	return str;
}

bool CollectionInterface::_is_dir_writable(const std::wstring& path)
{
	std::wstring tmpFileName = path;
	_wsDeleteTrailingNulls(tmpFileName);
	tmpFileName += L"\\~$TMPFILE.PRYRT";

	HANDLE hFile = CreateFile(tmpFileName.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		DWORD errNum = GetLastError();
		if (errNum != ERROR_ACCESS_DENIED) {
			std::wstring errmsg = L"_is_dir_writable::CreateFile[tmp](" + tmpFileName + L") failed: " + std::to_wstring(GetLastError()) + L"\n";
			throw wruntime_error(errmsg.c_str());
		}
		return false;
	}
	CloseHandle(hFile);
	DeleteFile(tmpFileName.c_str());
	return true;
}

std::wstring CollectionInterface::getWritableTempDir(void)
{
	// first try the system TEMP
	std::wstring tempDir(MAX_PATH+1, L'\0');
	GetTempPath(MAX_PATH + 1, const_cast<LPWSTR>(tempDir.data()));
	_wsDeleteTrailingNulls(tempDir);

	// if that fails, try c:\tmp or c:\temp
	if (!_is_dir_writable(tempDir)) {
		tempDir = L"c:\\temp";
		_wsDeleteTrailingNulls(tempDir);
	}
	if (!_is_dir_writable(tempDir)) {
		tempDir = L"c:\\tmp";
		_wsDeleteTrailingNulls(tempDir);
	}

	// if that fails, try the %USERPROFILE%
	if (!_is_dir_writable(tempDir)) {
		tempDir.resize(MAX_PATH + 1);
		if (!ExpandEnvironmentStrings(L"%USERPROFILE%", const_cast<LPWSTR>(tempDir.data()), MAX_PATH + 1)) {
			std::wstring errmsg = L"getWritableTempDir::ExpandEnvirontmentStrings(%USERPROFILE%) failed: " + std::to_wstring(GetLastError()) + L"\n";
			throw wruntime_error(errmsg.c_str());
		}
		_wsDeleteTrailingNulls(tempDir);
	}

	// last try: current directory
	if (!_is_dir_writable(tempDir)) {
		tempDir.resize(MAX_PATH + 1);
		GetCurrentDirectory(MAX_PATH + 1, const_cast<LPWSTR>(tempDir.data()));
		_wsDeleteTrailingNulls(tempDir);
	}

	// if that fails, no other ideas
	if (!_is_dir_writable(tempDir)) {
		std::wstring errmsg = L"getWritableTempDir() cannot find any writable directory; please make sure %TEMP% is defined and writable\n";
		throw wruntime_error(errmsg.c_str());
	}
	return tempDir;
}
