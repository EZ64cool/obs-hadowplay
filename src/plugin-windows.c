#include <util/bmem.h>
#include <util/dstr.h>
#include <util/platform.h>

#include <Windows.h>
#include <WinUser.h>
#include <winver.h>
#include <Psapi.h>

#include "plugin-macros.generated.h"

extern const char *dstr_find_last(struct dstr *src, char c);


#define SZ_STRING_FILE_INFO_W L"StringFileInfo"
#define SZ_PRODUCT_NAME_W L"ProductName"
#define SZ_HEX_LANG_ID_EN_GB_W L"0809"
#define SZ_HEX_CODE_PAGE_ID_UNICODE_W L"04E4"

#define test L"\\" SZ_STRING_FILE_INFO_W L"\\" SZ_HEX_LANG_ID_EN_US_W SZ_HEX_CODE_PAGE_ID_UNICODE_W L"\\" SZ_PRODUCT_NAME_W

bool GetProductName(const wchar_t* filepath, struct dstr* product_name)
{
	DWORD file_version_info_size =
		GetFileVersionInfoSizeExW(FILE_VER_GET_LOCALISED, filepath, NULL);

	if (file_version_info_size == 0)
	{
		return false;
	}

	LPVOID buffer = bmalloc(file_version_info_size);

	if (GetFileVersionInfoExW(FILE_VER_GET_LOCALISED, filepath, 0,
		file_version_info_size, buffer) == FALSE)
	{
		bfree(buffer);
		return false;
	}

	struct LANGANDCODEPAGE {
		WORD wLanguage;
		WORD wCodePage;
	} *lpTranslate;
	UINT cbTranslate;

	if (VerQueryValueW(buffer, L"\\VarFileInfo\\Translation",
		(LPVOID*)&lpTranslate, &cbTranslate) == FALSE || cbTranslate == 0)
	{
		bfree(buffer);
		return false;
	}

	LANGID language_id = GetUserDefaultUILanguage();
	wchar_t *key = bmalloc(50);

	bool set = false;

	for (int i = 0; i < (cbTranslate / sizeof(struct LANGANDCODEPAGE)); i++) {
		if (lpTranslate[i].wLanguage == language_id)
		{
			wsprintfW(key,
				  L"\\StringFileInfo\\%04x%04x\\ProductName",
				  language_id, lpTranslate[i].wCodePage);
			set = true;
			break;
		}
	}

	if (set == false)
	{
		wsprintfW(key, L"\\StringFileInfo\\%04x%04x\\ProductName",
			  lpTranslate[0].wLanguage, lpTranslate[0].wCodePage);
	}

	wchar_t *value;
	UINT value_length;

	if (VerQueryValueW(buffer, key, &value, &value_length) == FALSE || value == NULL)
	{
		bfree(buffer);
		return false;
	}

	bfree(buffer);

	dstr_from_wcs(product_name, value);

	return true;
}

bool GetWindowFilepath(HWND window, struct dstr* process_filepath)
{
	wchar_t *buffer = bmalloc(MAX_PATH * sizeof(wchar_t));

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
	//if (GetProcessImageFileName(hProc, buffer, MAX_PATH) == 0) {
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

struct dstr *GetFileName(struct dstr *filepath)
{
	const char *filename_start = dstr_find_last(filepath, '\\') + 1;

	const char *filename_end = strchr(filename_start, '.');

	struct dstr *filename = bmalloc(sizeof(struct dstr));
	dstr_init(filename);

	if (filename_start != NULL && filename_end != NULL) {
		size_t start = (filename_start - filepath->array);
		size_t count = (filename_end - filename_start);

		dstr_mid(filename, filepath, start, count);
	} else {
		dstr_copy_dstr(filename, filepath);
	}

	return filename;
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
};

BOOL EnumWindowsProc(HWND window, LPARAM param)
{
	struct dstr *str = (struct dstr *)param;

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

	if (GetMonitorInfo(monitor, &monitor_info) == false) {
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

	if (GetWindowFilepath(window, &filepath) == true) {

		struct dstr *filename = GetFileName(&filepath);

		struct dstr productName;
		dstr_init(&productName);

		GetProductName(dstr_to_wcs(&filepath), &productName);

		bool excluded = false;

		for (const char **vals = exclusions; *vals; vals++)
		{
			if (strcmpi(*vals, filename->array) == 0) {
				excluded = true;
				break;
			}
		}

		if (excluded == false) {

			if (dstr_is_empty(&productName) == false)
			{
				dstr_copy_dstr(str, &productName);
			} else {
				dstr_copy_dstr(str, filename);
			}
			found = true;
		}

		dstr_free(filename);
	}

	dstr_free(&filepath);

	return !found;
}

extern bool obs_hadowplay_get_fullscreen_window_name(struct dstr *process_name)
{
	return !EnumWindows(EnumWindowsProc, (LPARAM)process_name);
}