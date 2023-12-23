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

pthread_t update_thread;
struct dstr replay_target_name = {0};
struct dstr recording_target_name = {0};

void *obs_hadowplay_update(void *param)
{
	UNUSED_PARAMETER(param);

	char thread_name[64];
	snprintf(thread_name, 64, "%s update thread", PLUGIN_NAME);
	os_set_thread_name(thread_name);

	while (os_atomic_load_bool(&obs_hadowplay_module_loaded) == true) {

		obs_output_t *replay_output =
			obs_frontend_get_replay_buffer_output();

		if (replay_output != NULL &&
		    os_atomic_load_bool(&obs_hadowplay_manual_start) == false) {
			obs_output_release(replay_output);

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

		if (obs_frontend_replay_buffer_active() == true &&
		    dstr_is_empty(&replay_target_name)) {
			if (obs_hadowplay_get_fullscreen_window_name(
				    &replay_target_name) == true) {
				obs_log(LOG_INFO, "Replay target found: %s",
					replay_target_name.array);
			}
		}

		if (obs_frontend_recording_active() == true &&
		    dstr_is_empty(&recording_target_name)) {
			if (obs_hadowplay_get_fullscreen_window_name(
				    &recording_target_name) == true) {
				obs_log(LOG_INFO, "Recording target found: %s",
					recording_target_name.array);
			}
		}

		os_sleep_ms(1000);
	}

	return 0;
}

void obs_hadowplay_move_output_file(struct dstr *original_filepath,
				    struct dstr *target_name)
{
	const char *dir_start = strrchr(original_filepath->array, '/');

	struct dstr replay_filename;
	dstr_init_copy(&replay_filename, dir_start + 1);

	struct dstr file_dir;
	dstr_init(&file_dir);

	dstr_ncopy_dstr(&file_dir, original_filepath,
			(dir_start + 1) - original_filepath->array);

	dstr_cat_dstr(&file_dir, target_name);
	dstr_cat(&file_dir, "/");

	if (os_file_exists(file_dir.array) == false) {
		obs_log(LOG_INFO, "Creating directory: %s", file_dir.array);
		os_mkdir(file_dir.array);
	}

	struct dstr new_filepath;
	dstr_init_copy_dstr(&new_filepath, &file_dir);
	dstr_cat_dstr(&new_filepath, &replay_filename);

	obs_log(LOG_INFO, "Renaming files: %s -> %s", original_filepath->array,
		new_filepath.array);
	os_rename(original_filepath->array, new_filepath.array);

	dstr_free(&replay_filename);
	dstr_free(&file_dir);
	dstr_free(&new_filepath);
}

void obs_hadowplay_frontend_event_callback(enum obs_frontend_event event,
					   void *private_data)
{
	UNUSED_PARAMETER(private_data);

	switch (event) {
	case OBS_FRONTEND_EVENT_FINISHED_LOADING:
		int result = pthread_create(&update_thread, NULL,
					    obs_hadowplay_update, NULL);
		if (result != 0) {
			obs_log(LOG_ERROR,
				"Failed to create update thread (code %d), plugin is no longer able to track when to toggle the replay buffer",
				result);
		}
		break;

#pragma region Replay events
	case OBS_FRONTEND_EVENT_REPLAY_BUFFER_STOPPED:
		if (os_atomic_load_bool(&obs_hadowplay_is_replay_controlled) ==
		    true) {
			obs_log(LOG_INFO, "Replay buffer manually stopped");
			os_atomic_store_bool(&obs_hadowplay_manual_stop, true);
		}

		os_atomic_store_bool(&obs_hadowplay_is_replay_controlled,
				     false);
		os_atomic_store_bool(&obs_hadowplay_manual_start, false);
		break;

	case OBS_FRONTEND_EVENT_REPLAY_BUFFER_STARTED:
		if (os_atomic_load_bool(&obs_hadowplay_is_replay_controlled) ==
		    false) {
			os_atomic_store_bool(&obs_hadowplay_manual_start, true);
		}

		dstr_init(&replay_target_name);
		if (obs_hadowplay_get_fullscreen_window_name(
			    &replay_target_name) == true) {
			obs_log(LOG_INFO, "Replay target found: %s",
				replay_target_name.array);
		}
		break;

	case OBS_FRONTEND_EVENT_REPLAY_BUFFER_SAVED:
		if (dstr_is_empty(&replay_target_name) == true) {
			if (obs_hadowplay_get_fullscreen_window_name(
				    &replay_target_name) == true) {
				obs_log(LOG_INFO, "Replay target found: %s",
					replay_target_name.array);
			}
		}

		if (dstr_is_empty(&replay_target_name) == true) {
			return;
		}

		const char *replay_path_c = obs_frontend_get_last_replay();

		if (replay_path_c == NULL) {
			return;
		}

		struct dstr replay_path;
		dstr_init_copy(&replay_path, replay_path_c);

		obs_hadowplay_move_output_file(&replay_path,
					       &replay_target_name);

		dstr_free(&replay_path);
		break;
#pragma endregion Replay buffer events

#pragma region Recording events
	case OBS_FRONTEND_EVENT_RECORDING_STARTED:
		// Reset recording name for fresh recordings
		dstr_init(&recording_target_name);
		if (obs_hadowplay_get_fullscreen_window_name(
			    &recording_target_name) == true) {
			obs_log(LOG_INFO, "Recording target found: %s",
				recording_target_name.array);
		}
		break;

	case OBS_FRONTEND_EVENT_RECORDING_STOPPED:
		if (dstr_is_empty(&recording_target_name) == true) {
			if (obs_hadowplay_get_fullscreen_window_name(
				    &recording_target_name) == true) {
				obs_log(LOG_INFO, "Recording target found: %s",
					recording_target_name.array);
			}
		}

		if (dstr_is_empty(&recording_target_name) == true) {
			return;
		}

		const char *recording_path_c =
			obs_frontend_get_last_recording();

		if (recording_path_c == NULL) {
			return;
		}

		struct dstr recording_path;
		dstr_init_copy(&recording_path, recording_path_c);

		obs_hadowplay_move_output_file(&recording_path,
					       &recording_target_name);

		dstr_free(&recording_path);
		break;

#pragma endregion Recording events
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
