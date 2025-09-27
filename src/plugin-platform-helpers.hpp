#pragma once

#include <obs.h>
#include <util/dstr.h>
#include <string>

#if defined(_WIN32) || defined(_WIN64) || defined(__WINDOWS__) || \
	defined(__TOS_WIN__)
void obs_hadowplay_play_sound(const wchar_t *filepath);
#else
void obs_hadowplay_play_sound(const char *filepath);
#endif

#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC
#endif

std::string
obs_hadowplay_strip_executable_extension(const std::string &filename);

std::string obs_hadowplay_cleanup_path_string(const std::string &filename);

EXTERNC void obs_hadowplay_play_notif_sound();

EXTERNC bool obs_hadowplay_is_exe_excluded(const char *exe);

EXTERNC bool obs_hadowplay_show_notification(const std::string &title,
					     const std::string &message);

EXTERNC bool
obs_hadowplay_get_product_name_from_source(obs_source_t *source,
					   std::string &product_name);