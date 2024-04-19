#if defined(_WIN32) || defined(_WIN64) || defined(__WINDOWS__) || \
	defined(__TOS_WIN__)

#include <util/bmem.h>
#include <util/platform.h>
#include <util/dstr.h>

#include <Windows.h>
#include <WinUser.h>
#include <winver.h>
#include <Psapi.h>

#pragma comment(lib, "Winmm.lib")

#include "plugin-platform-helpers.hpp"
#include "config/config.hpp"

void obs_hadowplay_play_sound(const wchar_t *filepath)
{
	PlaySound(filepath, NULL, SND_FILENAME | SND_ASYNC);
}

bool win_get_product_name(const std::wstring &filepath,
			  std::string &product_name)
{
	DWORD temp = 0;
	const DWORD file_version_info_size = GetFileVersionInfoSizeExW(
		FILE_VER_GET_NEUTRAL, filepath.c_str(), &temp);

	if (file_version_info_size == 0) {
		return false;
	}

	const LPVOID buffer = bmalloc(file_version_info_size);

	if (buffer == NULL) {
		return false;
	}

	if (GetFileVersionInfoExW(FILE_VER_GET_NEUTRAL, filepath.c_str(), 0UL,
				  file_version_info_size, buffer) == FALSE) {
		bfree(buffer);
		return false;
	}

	struct LANGANDCODEPAGE {
		WORD wLanguage;
		WORD wCodePage;
	} *lpTranslate = 0;
	UINT cbTranslate = 0;

	if (VerQueryValueW(buffer, L"\\VarFileInfo\\Translation",
			   reinterpret_cast<void **>(&lpTranslate),
			   &cbTranslate) == FALSE ||
	    cbTranslate == 0) {
		bfree(buffer);
		return false;
	}

	const LANGID user_language_id = GetUserDefaultUILanguage();
	const UINT key_length = 50;
	wchar_t *key = reinterpret_cast<wchar_t *>(
		bmalloc(key_length * sizeof(wchar_t)));

	if (key == NULL) {
		bfree(buffer);
		return false;
	}

	WORD language_id = lpTranslate[0].wLanguage;
	WORD code_page_id = lpTranslate[0].wCodePage;

	for (int i = 0; i < (cbTranslate / sizeof(struct LANGANDCODEPAGE));
	     i++) {
		if (lpTranslate[i].wLanguage == user_language_id) {
			language_id = lpTranslate[i].wLanguage;
			code_page_id = lpTranslate[i].wCodePage;
			break;
		}
	}

	swprintf_s(key, key_length,
		   L"\\StringFileInfo\\%04x%04x\\FileDescription", language_id,
		   code_page_id);

	wchar_t *value = NULL;
	UINT value_length = 0;

	if (VerQueryValueW(buffer, key, reinterpret_cast<void **>(&value),
			   &value_length) == FALSE ||
	    value == NULL || value_length == 0) {
		swprintf_s(key, key_length,
			   L"\\StringFileInfo\\%04x%04x\\ProductName",
			   language_id, code_page_id);

		if (VerQueryValueW(buffer, key,
				   reinterpret_cast<void **>(&value),
				   &value_length) == FALSE ||
		    value == NULL || value_length == 0) {
			bfree(key);
			bfree(buffer);
			return false;
		}
	}

	bfree(key);
	bfree(buffer);

	char *product_name_c = nullptr;
	os_wcs_to_utf8_ptr(value, value_length, &product_name_c);

	product_name = std::string(product_name_c);
	bfree(product_name_c);

	return true;
}

bool win_get_window_filepath(HWND window, std::wstring &process_filepath)
{
	wchar_t *buffer = reinterpret_cast<wchar_t *>(
		bmalloc(MAX_PATH * sizeof(wchar_t)));

	if (buffer == NULL) {
		bfree(buffer);
		return false;
	}

	DWORD dwProcId = 0;
	if (GetWindowThreadProcessId(window, &dwProcId) == 0) {
		bfree(buffer);
		return false;
	}

	HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
				   FALSE, dwProcId);

	DWORD filepath_length = MAX_PATH;
	if (QueryFullProcessImageNameW(hProc, 0, buffer, &filepath_length) ==
	    0) {
		CloseHandle(hProc);
		bfree(buffer);
		return false;
	}
	CloseHandle(hProc);

	process_filepath = std::wstring(buffer);

	bfree(buffer);

	return true;
}

std::string
obs_hadowplay_strip_executable_extension(const std::string &filename)
{
	const char *ext = os_get_path_extension(filename.c_str());
	if (ext != nullptr && strcmpi(ext, ".exe") == 0) {
		return filename.substr(0, ext - filename.c_str());
	}
	return filename;
}

bool obs_hadowplay_wstring_ends_with(const std::wstring &string,
				     const std::wstring &end)
{
	if (string.length() >= end.length()) {
		return (string.compare(string.length() - end.length(),
				       end.length(), end) == 0);
	}
	return false;
}

HWND obs_hadowplay_find_window(const std::wstring &win_class,
			       const std::wstring &exe)
{
	HWND window = FindWindowW(win_class.c_str(), nullptr);

	while (window != NULL) {
		std::wstring filepath;
		if (win_get_window_filepath(window, filepath) &&
		    obs_hadowplay_wstring_ends_with(filepath, exe)) {
			return window;
		}

		window =
			FindWindowExW(NULL, window, win_class.c_str(), nullptr);
	}

	return NULL;
}

bool obs_hadowplay_get_product_name_from_source(obs_source_t *source,
						std::string &product_name)
{
	if (source == nullptr)
		return false;

	calldata_t hooked_calldata;
	calldata_init(&hooked_calldata);

	proc_handler_t *source_proc_handler =
		obs_source_get_proc_handler(source);
	if (proc_handler_call(source_proc_handler, "get_hooked",
			      &hooked_calldata) == false) {
		calldata_free(&hooked_calldata);
		return false;
	}

	const char *exe = calldata_string(&hooked_calldata, "executable");
	const char *win_class = calldata_string(&hooked_calldata, "class");

	wchar_t *win_class_w = nullptr;
	os_utf8_to_wcs_ptr(win_class, strlen(win_class), &win_class_w);
	wchar_t *exe_w = nullptr;
	os_utf8_to_wcs_ptr(exe, strlen(exe), &exe_w);

	HWND window = obs_hadowplay_find_window(win_class_w, exe_w);

	bfree(win_class_w);
	bfree(exe_w);

	std::wstring filepath;

	if (win_get_window_filepath(window, filepath) == true) {
		if (win_get_product_name(filepath, product_name) == true) {
			calldata_free(&hooked_calldata);
			return true;
		}
	}

	product_name = obs_hadowplay_strip_executable_extension(exe);

	calldata_free(&hooked_calldata);

	return false;
}

bool obs_hadowplay_is_exe_excluded(const char *exe)
{
	if (exe == nullptr)
		return true;

	std::string exe_str = obs_hadowplay_strip_executable_extension(exe);

	for (std::string val : Config::Inst().m_exclusions) {
		if (strcmpi(val.c_str(), exe_str.c_str()) == 0) {
			return true;
		}
	}

	return false;
}
#endif
