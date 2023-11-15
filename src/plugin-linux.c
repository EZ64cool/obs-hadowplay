#if defined(__linux__) || defined(__linux) || defined(linux) || \
	defined(__gnu_linux__)

#include <util/dstr.h>

extern bool obs_hadowplay_get_fullscreen_window_name(struct dstr *process_name)
{
	UNUSED_PARAMETER(process_name);
	return false;
}

#endif