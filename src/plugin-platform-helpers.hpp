#pragma once

#include <obs.h>
#include <util/dstr.h>
#include <string>

#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC
#endif

EXTERNC void obs_hadowplay_play_notif_sound();

EXTERNC bool obs_hadowplay_is_exe_excluded(const char *exe);

EXTERNC bool obs_hadowplay_show_notification(std::string &title,
					     std::string &message);

EXTERNC bool
obs_hadowplay_get_product_name_from_source(obs_source_t *source,
					   std::string &product_name);