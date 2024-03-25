#pragma once

#include <util/dstr.h>

#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC
#endif

EXTERNC bool dstr_get_filename(struct dstr *filepath, struct dstr *filename);

EXTERNC void obs_hadowplay_play_notif_sound();

EXTERNC void *obs_hadoowplay_print_window(const char *cls);

EXTERNC bool obs_hadowplay_show_notification(struct dstr *title,
					     struct dstr *message);
