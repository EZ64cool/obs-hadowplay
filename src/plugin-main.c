/*
Plugin Name
Copyright (C) <Year> <Developer> <Email Address>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

#include <obs.h>
#include <obs-module.h>
#include <obs-frontend-api.h>
#include <obs-source.h>
#include <util/platform.h>
#include <util/threading.h>
#include <util/dstr.h>

#include "plugin-support.h"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

void obs_hadowplay_consume_enum_source(obs_source_t *parent,
				       obs_source_t *source, void *param)
{
	UNUSED_PARAMETER(parent);

	obs_source_t **active_game_capture = param;

	if (*active_game_capture != NULL)
		return;

	const char *id = obs_source_get_id(source);

	if (strcmp(id, "game_capture") == 0) {

		uint32_t width = obs_source_get_width(source);
		uint32_t height = obs_source_get_height(source);

		bool is_active = width > 0 && height > 0;

		if (is_active) {
			*active_game_capture = obs_source_get_ref(source);
		}
	}
}

bool obs_hadowplay_is_replay_controlled = false;
bool obs_hadowplay_manual_start = false;
bool obs_hadowplay_manual_stop = false;
bool obs_hadowplay_module_loaded = false;

extern bool obs_hadowplay_get_fullscreen_window_name(struct dstr *process_name);

void *obs_hadowplay_update(void *param)
{
	UNUSED_PARAMETER(param);

	char thread_name[64];
	snprintf(thread_name, 64, "%s update thread", PLUGIN_NAME);
	os_set_thread_name(thread_name);

	while (os_atomic_load_bool(&obs_hadowplay_module_loaded) == true) {

		if (os_atomic_load_bool(&obs_hadowplay_manual_start) == false) {
			obs_source_t *scene_source =
				obs_frontend_get_current_scene();

			obs_source_t *game_capture_source = NULL;

			obs_source_enum_active_sources(
				scene_source, obs_hadowplay_consume_enum_source,
				&game_capture_source);

			if (game_capture_source != NULL) {
				if (obs_frontend_replay_buffer_active() ==
					    false &&
				    os_atomic_load_bool(
					    &obs_hadowplay_manual_stop) ==
					    false) {
					const char *source_name =
						obs_source_get_name(
							game_capture_source);
					obs_log(LOG_INFO,
						"Active game capture found: %s",
						source_name);
					os_atomic_store_bool(
						&obs_hadowplay_is_replay_controlled,
						true);
					obs_frontend_replay_buffer_start();
					obs_log(LOG_INFO,
						"Replay buffer started");
				}

				obs_source_release(game_capture_source);
			} else if (os_atomic_load_bool(
					   &obs_hadowplay_is_replay_controlled) ==
					   true &&
				   obs_frontend_replay_buffer_active() ==
					   true) {
				obs_log(LOG_INFO,
					"No active game capture found");
				obs_frontend_replay_buffer_stop();
				obs_log(LOG_INFO, "Replay buffer stopped");

				os_atomic_store_bool(
					&obs_hadowplay_is_replay_controlled,
					false);
			} else if (obs_frontend_replay_buffer_active() ==
				   false) {
				os_atomic_store_bool(&obs_hadowplay_manual_stop,
						     false);
			}

			obs_source_release(scene_source);
		}

		os_sleep_ms(1000);
	}

	return 0;
}

pthread_t update_thread;
struct dstr replay_target_name = {0};

void obs_hadowplay_frontend_event_callback(enum obs_frontend_event event,
					   void *private_data)
{
	UNUSED_PARAMETER(private_data);

	if (event == OBS_FRONTEND_EVENT_FINISHED_LOADING) {
		int result = pthread_create(&update_thread, NULL,
					    obs_hadowplay_update, NULL);
		if (result != 0) {
			obs_log(LOG_ERROR,
				"Failed to create update thread (code %d), plugin is no longer able to track when to toggle the replay buffer",
				result);
		}
	} else if (event == OBS_FRONTEND_EVENT_REPLAY_BUFFER_STOPPED) {
		if (os_atomic_load_bool(&obs_hadowplay_is_replay_controlled) ==
		    true) {
			obs_log(LOG_INFO, "Replay buffer manually stopped");
			os_atomic_store_bool(&obs_hadowplay_manual_stop, true);
		}

		os_atomic_store_bool(&obs_hadowplay_is_replay_controlled,
				     false);
		os_atomic_store_bool(&obs_hadowplay_manual_start, false);
	} else if (event == OBS_FRONTEND_EVENT_REPLAY_BUFFER_STARTED) {
		if (os_atomic_load_bool(&obs_hadowplay_is_replay_controlled) ==
		    false) {
			os_atomic_store_bool(&obs_hadowplay_manual_start, true);
		}

		obs_hadowplay_get_fullscreen_window_name(&replay_target_name);
	} else if (event == OBS_FRONTEND_EVENT_REPLAY_BUFFER_SAVED ||
		   event == OBS_FRONTEND_EVENT_RECORDING_STOPPED) {
		if (dstr_is_empty(&replay_target_name) == true) {
			obs_hadowplay_get_fullscreen_window_name(
				&replay_target_name);
		}

		if (dstr_is_empty(&replay_target_name) == true) {
			return;
		}

		const char *replay_path_c = NULL;

		if (event == OBS_FRONTEND_EVENT_RECORDING_STOPPED) {
			replay_path_c = obs_frontend_get_last_recording();
		} else {
			replay_path_c = obs_frontend_get_last_replay();
		}

		if (replay_path_c == NULL) {
			return;
		}

		struct dstr replay_path;
		dstr_init_copy(&replay_path, replay_path_c);

		const char *dir_start = strrchr(replay_path.array, '/');

		struct dstr replay_filename;
		dstr_init_copy(&replay_filename, dir_start + 1);

		struct dstr replay_dir;
		dstr_init(&replay_dir);

		dstr_ncopy_dstr(&replay_dir, &replay_path,
				(dir_start + 1) - replay_path.array);

		dstr_cat_dstr(&replay_dir, &replay_target_name);
		dstr_cat(&replay_dir, "/");

		if (os_file_exists(replay_dir.array) == false) {
			obs_log(LOG_INFO, "Creating directory: %s",
				replay_dir.array);
			os_mkdir(replay_dir.array);
		}

		struct dstr new_replay_path;
		dstr_init_copy_dstr(&new_replay_path, &replay_dir);
		dstr_cat_dstr(&new_replay_path, &replay_filename);

		obs_log(LOG_INFO, "Renaming files: %s -> %s", replay_path.array,
			new_replay_path.array);
		os_rename(replay_path.array, new_replay_path.array);

		dstr_free(&replay_path);
		dstr_free(&replay_filename);
		dstr_free(&replay_dir);
		dstr_free(&new_replay_path);
	}
}

bool obs_module_load(void)
{
	// No need to be atomic since the thread hasn't started yet.
	obs_hadowplay_module_loaded = true;

	obs_frontend_add_event_callback(obs_hadowplay_frontend_event_callback,
					NULL);

	obs_log(LOG_INFO, "plugin loaded successfully (version %s)",
		PLUGIN_VERSION);

	return true;
}

void obs_module_unload()
{
	os_atomic_store_bool(&obs_hadowplay_module_loaded, false);

	obs_log(LOG_INFO, "Awaiting update thread closure");
	void *return_val = NULL;
	int result = pthread_join(update_thread, &return_val);
	if (result == 0) {
		obs_log(LOG_INFO, "Update thread closed");
	} else {
		obs_log(LOG_ERROR, "Failed to join update thread: %d", result);
	}

	obs_log(LOG_INFO, "plugin unloaded");
}