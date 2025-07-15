#if defined(__linux__) || defined(__linux) || defined(linux) || \
	defined(__gnu_linux__)

#include <util/dstr.h>

#include "plugin-platform-helpers.hpp"

extern bool obs_hadowplay_get_fullscreen_window_name(struct dstr *process_name)
{
	UNUSED_PARAMETER(process_name);
	return false;
}

extern bool obs_hadowplay_is_capture_source(obs_source_t *source)
{
	return (source != nullptr &&
		(strcasecmp(obs_source_get_id(source), "xcomposite_input") == 0 ||
		 strcasecmp(obs_source_get_id(source), "vkcapture-source") == 0 ||
		 strcasecmp(obs_source_get_id(source), "xshm_input") == 0 ||
		 strcasecmp(obs_source_get_id(source), "xshm_input_v2") == 0));
}

#endif
