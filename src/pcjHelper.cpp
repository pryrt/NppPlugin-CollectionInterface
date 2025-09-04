#include "pcjHelper.h"
#include <pathcch.h>
#include <shlwapi.h>

namespace pcjHelper
{
	// delete null characters from padded wstrings
	std::wstring delNull(std::wstring& str)
	{
		const auto pos = str.find(L'\0');
		if (pos != std::wstring::npos) {
			str.erase(pos);
		}
		return str;
	};

	// delete null characters from padded wstrings
	std::string delNull(std::string& str)
	{
		const auto pos = str.find('\0');
		if (pos != std::string::npos) {
			str.erase(pos);
		}
		return str;
	};

	// convert wstring to UTF8-encoded bytes in std::string
	std::string wstring_to_utf8(std::wstring& wstr)
	{
		if (wstr.empty()) return std::string();
		int szNeeded = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), static_cast<int>(wstr.size()), NULL, 0, NULL, NULL);
		if (szNeeded == 0) return std::string();
		std::string str(szNeeded, '\0');
		int result = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), static_cast<int>(wstr.size()), const_cast<LPSTR>(str.data()), szNeeded, NULL, NULL);
		if (result == 0) return std::string();
		return str;
	}

	// convert UTF8-encoded bytes in std::string to std::wstring
	std::wstring utf8_to_wstring(std::string& str)
	{
		if (str.empty()) return std::wstring();
		int szNeeded = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(str.size()), NULL, 0);
		if (szNeeded == 0) return std::wstring();
		std::wstring wstr(szNeeded, L'\0');
		int result = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(str.size()), const_cast<LPWSTR>(wstr.data()), szNeeded);
		if (result == 0) return std::wstring();
		return wstr;
	}

	// recursively create all levels necessary for a given directory
	BOOL RecursiveCreateDirectory(std::wstring wsPath)
	{
		std::wstring wsParent = wsPath;
		PathRemoveFileSpec(const_cast<LPWSTR>(wsParent.data()));
		if (!PathFileExists(wsParent.c_str())) {
			BOOL stat = RecursiveCreateDirectory(wsParent);
			if (!stat) return stat;
		}
		return CreateDirectory(wsPath.c_str(), NULL);
	}

	// checks if a given directory is writeable
	bool is_dir_writable(const std::wstring& path)
	{
		// first, grab the directory and make sure it exists
		std::wstring tmpFileDir = path;
		pcjHelper::delNull(tmpFileDir);

		if (!PathFileExists(tmpFileDir.c_str())) {
			BOOL stat = RecursiveCreateDirectory(tmpFileDir);
			if (!stat) {
				DWORD errNum = GetLastError();
				if (errNum != ERROR_ACCESS_DENIED) {
					std::wstring errmsg = L"Could not find or create directory for \"" + path + L"\": " + std::to_wstring(GetLastError()) + L"\n";
					::MessageBox(NULL, errmsg.c_str(), L"Directory error", MB_ICONERROR);
				}
				return false;
			}
		}

		// once it exists, move on to trying to write a file in that directory
		std::wstring tmpFileName = tmpFileDir + L"\\~$TMPFILE.PRYRT";

		HANDLE hFile = CreateFile(tmpFileName.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile == INVALID_HANDLE_VALUE) {
			DWORD errNum = GetLastError();
			if (errNum != ERROR_ACCESS_DENIED) {
				std::wstring errmsg = L"Error when testing if \"" + path + L"\" is writeable: " + std::to_wstring(GetLastError()) + L"\n";
				::MessageBox(NULL, errmsg.c_str(), L"Directory error", MB_ICONERROR);
			}
			return false;
		}
		CloseHandle(hFile);
		DeleteFile(tmpFileName.c_str());
		return true;
	}


};
