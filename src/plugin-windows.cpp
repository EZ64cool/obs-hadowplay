#if defined(_WIN32) || defined(_WIN64) || defined(__WINDOWS__) || \
	defined(__TOS_WIN__)

#include <util/bmem.h>
#include <util/platform.h>
#include <util/dstr.h>

#include <Windows.h>
#include <WinUser.h>
#include <winver.h>
#include <Psapi.h>

#include "config/config.hpp"

extern "C" bool dstr_get_filename(struct dstr *filepath, struct dstr *filename);

bool win_get_product_name(const struct dstr *filepath,
			  struct dstr *product_name)
{
	wchar_t *const w_filename = dstr_to_wcs(filepath);

	DWORD temp = 0;
	const DWORD file_version_info_size = GetFileVersionInfoSizeExW(
		FILE_VER_GET_NEUTRAL, w_filename, &temp);

	if (file_version_info_size == 0) {
		bfree(w_filename);
		return false;
	}

	const LPVOID buffer = bmalloc(file_version_info_size);

	if (buffer == NULL) {
		bfree(w_filename);
		return false;
	}

	if (GetFileVersionInfoExW(FILE_VER_GET_NEUTRAL, w_filename, 0UL,
				  file_version_info_size, buffer) == FALSE) {
		bfree(w_filename);
		bfree(buffer);
		return false;
	}

	bfree(w_filename);

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

	dstr_from_wcs(product_name, value);

	return true;
}

bool win_get_window_filepath(HWND window, struct dstr *process_filepath)
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

	dstr_from_wcs(process_filepath, buffer);

	bfree(buffer);

	return true;
}

static const char *exclusions[] = {
	"explorer",
	"steam",
	"battle.net",
	"galaxyclient",
	"skype",
	"uplay",
	"origin",
	"devenv",
	"taskmgr",
	"chrome",
	"discord",
	"firefox",
	"systemsettings",
	"applicationframehost",
	"cmd",
	"shellexperiencehost",
	"searchui",
	"lockapp",
	"obs",
	"TextInputHost",
	"NVIDIA Share",
	NULL,
};

bool obs_hadowplay_is_excluded(struct dstr &window_name)
{
	for (const char **vals = exclusions; *vals; vals++) {
		if (strcmpi(*vals, window_name.array) == 0) {
			return true;
		}
	}

	for (std::string val : Config::Inst().m_exclusions) {
		if (strcmpi(val.c_str(), window_name.array) == 0) {
			return true;
		}
	}

	return false;
}

BOOL win_enum_windows(HWND window, LPARAM param)
{
	struct dstr *str = reinterpret_cast<struct dstr *>(param);

	if (IsWindowVisible(window) == false)
		return true;

	RECT window_rect = {0};
	if (GetWindowRect(window, &window_rect) == false) {
		return true;
	}

	HMONITOR monitor = MonitorFromWindow(window, MONITOR_DEFAULTTONULL);

	if (monitor == NULL) {
		return true;
	}

	MONITORINFO monitor_info = {0};
	monitor_info.cbSize = sizeof(MONITORINFO);

	if (GetMonitorInfoW(monitor, &monitor_info) == false) {
		return true;
	}

	if (monitor_info.rcMonitor.left != window_rect.left ||
	    monitor_info.rcMonitor.right != window_rect.right ||
	    monitor_info.rcMonitor.top != window_rect.top ||
	    monitor_info.rcMonitor.bottom != window_rect.bottom) {
		return true;
	}

	struct dstr filepath;
	dstr_init(&filepath);

	bool found = false;

	if (win_get_window_filepath(window, &filepath) == true) {

		struct dstr filename;
		dstr_init(&filename);

		if (dstr_get_filename(&filepath, &filename) == false) {
			return true;
		}

		struct dstr product_name;
		dstr_init(&product_name);

		win_get_product_name(&filepath, &product_name);

		bool excluded = obs_hadowplay_is_excluded(filename);

		if (excluded == false) {
			if (str != NULL) {
				if (dstr_is_empty(&product_name) == false) {
					dstr_copy_dstr(str, &product_name);
				} else {
					dstr_copy_dstr(str, &filename);
				}
			}
			found = true;
		}

		dstr_free(&filename);
	}

	dstr_free(&filepath);

	return !found;
}

extern "C" bool
obs_hadowplay_get_fullscreen_window_name(struct dstr *process_name)
{
	return !EnumWindows(win_enum_windows, (LPARAM)process_name);
}
#endif
