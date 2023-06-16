#include <util/bmem.h>
#include <util/dstr.h>
#include <util/platform.h>

#include <Windows.h>
#include <WinUser.h>
#include <Psapi.h>

extern const char *dstr_find_last(struct dstr *src, char c);

bool GetWindowName(HWND window, struct dstr *process_name)
{
	wchar_t *buffer = bmalloc(MAX_PATH * sizeof(wchar_t));

	if (buffer == NULL)
		return false;

	DWORD dwProcId = 0;
	if (GetWindowThreadProcessId(window, &dwProcId) == 0)
		return false;

	HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
				   FALSE, dwProcId);

	if (GetProcessImageFileName(hProc, buffer, MAX_PATH) == 0) {
		CloseHandle(hProc);
		return false;
	}
	CloseHandle(hProc);

	struct dstr process_path;
	dstr_init(&process_path);

	dstr_from_wcs(&process_path, buffer);

	const char *filename_start = dstr_find_last(&process_path, '\\') + 1;

	const char *filename_end = strchr(filename_start, '.');

	if (filename_start != NULL && filename_end != NULL) {
		size_t start = (filename_start - process_path.array);
		size_t count = (filename_end - filename_start);

		dstr_mid(process_name, &process_path, start, count);
	} else {
		dstr_copy_dstr(process_name, &process_path);
	}

	dstr_free(&process_path);

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
	//"Plex"
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

	struct dstr temp;
	dstr_init(&temp);

	bool found = false;

	if (GetWindowName(window, &temp) == true) {

		bool excluded = false;

		for (const char **vals = exclusions; *vals; vals++)
		{
			if (strcmpi(*vals, temp.array) == 0) {
				excluded = true;
				break;
			}
		}

		if (excluded == false) {

			dstr_copy_dstr(str, &temp);
			found = true;
		}
	}

	dstr_free(&temp);

	return !found;
}

extern bool obs_hadowplay_get_fullscreen_window_name(struct dstr *process_name)
{
	return !EnumWindows(EnumWindowsProc, (LPARAM)process_name);
}