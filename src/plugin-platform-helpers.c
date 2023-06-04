#include <util/bmem.h>
#include <util/dstr.h>

#include <Windows.h>
#include <WinUser.h>
#include <Psapi.h>

const char* dstr_find_last(struct dstr* src, char c)
{
	int i = 0;
	const char *pos = NULL;
	while (i < src->len)
	{
		if (src->array[i] == c)
		{
			pos = &src->array[i];
		}

		i++;
	}

	return pos;
}

bool GetCurrentForegroundProcessName(struct dstr* process_name)
{
	wchar_t *buffer = bmalloc(MAX_PATH * sizeof(wchar_t));

	if (buffer == NULL)
		return false;

	HWND foreground_window = GetForegroundWindow();

	DWORD dwProcId = 0;
	if (GetWindowThreadProcessId(foreground_window, &dwProcId) == 0)
		return false;

	HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
				   FALSE, dwProcId);

	if (GetProcessImageFileName(hProc, buffer, MAX_PATH) == 0)
		return 0;
	CloseHandle(hProc);

	dstr_init(process_name);

	struct dstr process_path;
	dstr_init(&process_path);

	dstr_from_wcs(&process_path, buffer);

	const char* filename_start = dstr_find_last(&process_path, '\\');

	const char *filename_end = dstr_find(&process_path, ".");

	dstr_mid(process_name, &process_path,
		 (filename_start - process_path.array) + 1,
		 (filename_end - filename_start) - 1);

	bfree(buffer);

	return true;
}