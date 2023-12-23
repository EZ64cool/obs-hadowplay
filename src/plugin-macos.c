#if defined(__APPLE__) || defined(__APPLE_CC__) || defined(__OSX__)

#include <util/dstr.h>

extern bool obs_hadowplay_get_fullscreen_window_name(struct dstr *process_name)
{
	UNUSED_PARAMETER(process_name);
	return false;
}
#endif
