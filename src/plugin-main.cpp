/*
obs-hadowplay
Copyright (C) 2023 EZ64cool

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
#include "plugin-platform-helpers.hpp"
#include "ui/SettingsDialog.hpp"
#include "config/config.hpp"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

#define TEXT_SETTINGS_MENU obs_module_text("OBSHadowplay.Settings")

bool obs_hadowplay_is_capture_source_hooked(obs_source_t *source)
{
	calldata_t hooked_calldata;
	calldata_init(&hooked_calldata);

	proc_handler_t *source_proc_handler =
		obs_source_get_proc_handler(source);
	proc_handler_call(source_proc_handler, "get_hooked", &hooked_calldata);

	bool hooked = calldata_bool(&hooked_calldata, "hooked");
	const char *exe = calldata_string(&hooked_calldata, "executable");
	hooked &= obs_hadowplay_is_exe_excluded(exe) == false;
	// Check for width & height, as window captures can stay hooked when hidden
	if (hooked) {
		hooked &= (obs_source_get_width(source) != 0 &&
			   obs_source_get_height(source) != 0);
	}

	calldata_free(&hooked_calldata);

	return hooked;
}

void obs_hadowplay_enum_source_for_hooked_capture(obs_source_t *parent,
						  obs_source_t *source,
						  void *param)
{
	UNUSED_PARAMETER(parent);

	obs_source_t **active_game_capture =
		reinterpret_cast<obs_source_t **>(param);

	if (*active_game_capture != nullptr)
		return;

	if (obs_hadowplay_is_capture_source(source) &&
	    obs_hadowplay_is_capture_source_hooked(source)) {
		*active_game_capture = obs_source_get_ref(source);
	}
}

bool obs_hadowplay_get_captured_name(std::string &target_name)
{
	obs_source_t *scene_source = obs_frontend_get_current_scene();

	obs_source_t *active_capture_source = nullptr;

	obs_source_enum_active_tree(
		scene_source, &obs_hadowplay_enum_source_for_hooked_capture,
		&active_capture_source);

	if (active_capture_source != nullptr) {
		obs_log(LOG_INFO, "Active capture found: %s",
			obs_source_get_name(active_capture_source));

		obs_hadowplay_get_product_name_from_source(
			active_capture_source, target_name);
		obs_source_release(active_capture_source);
		obs_source_release(scene_source);
		return true;
	}

	obs_source_release(scene_source);

	return false;
}

bool obs_hadowplay_is_replay_controlled = false;
bool obs_hadowplay_manual_start = false;
bool obs_hadowplay_manual_stop = false;

template<typename... Args>
std::string obs_hadowplay_string_format(const std::string &format, Args... args)
{
	int size_s = std::snprintf(nullptr, 0, format.c_str(), args...) +
		     1; // Extra space for '\0'
	auto size = static_cast<size_t>(size_s);
	std::unique_ptr<char[]> buf(new char[size]{});
	std::snprintf(buf.get(), size, format.c_str(), args...);
	return std::string(buf.get(),
			   buf.get() + size -
				   1); // We don't want the '\0' inside
}

std::string
obs_hadowplay_format_output_filename(const std::string &original_filename,
				     const std::string &target_name)
{
	if (Config::Inst().m_use_custom_filename_format == true) {
		std::string extension =
			os_get_path_extension(original_filename.c_str());

		size_t extension_start = original_filename.rfind(extension);

		std::string original_name =
			original_filename.substr(0, extension_start);

		const static std::string stringReplacement = std::string("%s");

		std::string format =
			stringReplacement +
			Config::Inst().m_custom_filename_seperator +
			stringReplacement;

		// Default FilenameArrangment of TargetBefore
		std::string first_argument =
			obs_hadowplay_cleanup_path_string(target_name);
		std::string second_argument = original_name;

		if (Config::Inst().m_custom_filename_arrangement ==
		    TargetAfter) {
			second_argument = first_argument;
			first_argument = original_name;
		}

		return obs_hadowplay_cleanup_path_string(
			obs_hadowplay_string_format(format,
						    first_argument.c_str(),
						    second_argument.c_str())
				.append(extension));
	} else {
		return original_filename;
	}
}

std::string obs_hadowplay_move_output_file(const std::string &original_filepath,
					   const std::string &target_name)
{
	const size_t filename_pos = original_filepath.find_last_of("/\\") + 1;

	std::string replay_filename = original_filepath.substr(filename_pos);

	std::string target_directory =
		original_filepath.substr(0, filename_pos);

	if (Config::Inst().m_enable_folder_organisation) {
		target_directory =
			target_directory +
			obs_hadowplay_cleanup_path_string(target_name);

		if (os_file_exists(target_directory.c_str()) == false) {
			obs_log(LOG_INFO, "Creating directory: %s",
				target_directory.c_str());
			if (os_mkdir(target_directory.c_str()) != 0) {
				obs_log(LOG_ERROR,
					"Failed to create directory: errno %i",
					errno);
			} else {
				obs_log(LOG_INFO,
					"Succesfully created directory: %s",
					target_directory.c_str());
			}
		}
	}

	replay_filename = obs_hadowplay_format_output_filename(replay_filename,
							       target_name);

	std::string new_filepath = target_directory + "/" + replay_filename;

	obs_log(LOG_INFO, "Renaming file: %s -> %s", original_filepath.c_str(),
		new_filepath.c_str());

	if (os_rename(original_filepath.c_str(), new_filepath.c_str()) != 0) {
		obs_log(LOG_ERROR, "Failed to rename file: errno %i", errno);
	} else {
		obs_log(LOG_INFO, "Successfully renamed file: %s -> %s",
			original_filepath.c_str(), new_filepath.c_str());
	}

	return new_filepath;
}

void obs_hadowplay_notify_saved(const std::string &title,
				const std::string &filepath)
{
	if (Config::Inst().m_play_notif_sound == true) {
		obs_hadowplay_play_notif_sound();
	}
	if (Config::Inst().m_show_desktop_notif == true) {
		obs_hadowplay_show_notification(title, filepath);
	}
}

using obs_hadowplay_time_point =
	std::chrono::time_point<std::chrono::steady_clock>;

obs_hadowplay_time_point requested_stop;

bool obs_hadowplay_start_automatic_replay_buffer()
{
	// False if auto replay disabled
	if (Config::Inst().m_auto_replay_buffer == false)
		return false;

	// False if replay buffer is disabled
	obs_output_t *replay_output = obs_frontend_get_replay_buffer_output();
	if (replay_output == nullptr)
		return false;

	obs_output_release(replay_output);

	// Reset stop time point
	requested_stop = obs_hadowplay_time_point();

	// False if already running
	if (obs_frontend_replay_buffer_active() == true)
		return false;

	// False if manually stopped
	if (os_atomic_load_bool(&obs_hadowplay_manual_stop) == true)
		return false;

	obs_log(LOG_INFO, "Automatic replay started");
	os_atomic_store_bool(&obs_hadowplay_is_replay_controlled, true);
	obs_frontend_replay_buffer_start();

	return true;
}

bool obs_hadowplay_stop_automatic_replay_buffer(bool force = false)
{
	if (Config::Inst().m_auto_replay_buffer_stop_delay != 0 &&
	    force == false) {
		obs_hadowplay_time_point current_time =
			std::chrono::steady_clock::now();

		if (requested_stop.time_since_epoch().count() == 0) {
			// Start point for delay timer
			requested_stop = current_time;
			return false;
		} else if (std::chrono::duration_cast<std::chrono::seconds>(
				   current_time - requested_stop)
				   .count() <
			   Config::Inst().m_auto_replay_buffer_stop_delay) {
			return false;
		}
	}

	// Clear manual stop if automatic stop is called
	os_atomic_store_bool(&obs_hadowplay_manual_stop, false);

	// False if not running
	if (obs_frontend_replay_buffer_active() == false)
		return false;

	// False if manually started
	if (os_atomic_load_bool(&obs_hadowplay_manual_start) == true &&
	    force == false)
		return false;

	obs_log(LOG_INFO, "Automatic replay stopped");

	os_atomic_store_bool(&obs_hadowplay_is_replay_controlled, false);
	obs_frontend_replay_buffer_stop();

	return true;
}

void obs_hadowplay_default_replay_buffer_event_callback(
	enum obs_frontend_event event, void *private_data);

void obs_hadowplay_replay_buffer_deactivated(void *data, calldata_t *calldata)
{
	UNUSED_PARAMETER(data);

	obs_frontend_add_event_callback(
		obs_hadowplay_default_replay_buffer_event_callback, NULL);

	obs_output_t *output = (obs_output_t *)calldata_ptr(calldata, "output");

	signal_handler_t *signal_handler =
		obs_output_get_signal_handler(output);

	signal_handler_disconnect(signal_handler, "deactivate",
				  &obs_hadowplay_replay_buffer_deactivated,
				  nullptr);

	obs_log(LOG_INFO, "Replay buffer successfully restarted");

	obs_frontend_replay_buffer_start();
}

void obs_hadowplay_restart_replay_buffer()
{
	obs_log(LOG_INFO, "Restarting replay buffer");

	obs_frontend_remove_event_callback(
		obs_hadowplay_default_replay_buffer_event_callback, NULL);

	obs_output_t *replay_buffer = obs_frontend_get_replay_buffer_output();

	signal_handler_t *signal_handler =
		obs_output_get_signal_handler(replay_buffer);

	signal_handler_connect(signal_handler, "deactivate",
			       &obs_hadowplay_replay_buffer_deactivated,
			       nullptr);

	obs_output_force_stop(replay_buffer);

	obs_output_release(replay_buffer);
}

extern void obs_hadowplay_replay_buffer_stop()
{
	obs_hadowplay_stop_automatic_replay_buffer();
}

bool obs_hadowplay_has_active_captures()
{
	obs_source_t *scene_source = obs_frontend_get_current_scene();

	obs_source_t *hooked_capture_source = nullptr;

	obs_source_enum_active_tree(
		scene_source, obs_hadowplay_enum_source_for_hooked_capture,
		&hooked_capture_source);

	bool has_active_capture = hooked_capture_source != nullptr;

	obs_source_release(hooked_capture_source);
	obs_source_release(scene_source);

	return has_active_capture;
}

bool obs_hadowplay_refresh_automatic_replay_buffer()
{
	// Early out if auto replay disabled
	if (Config::Inst().m_auto_replay_buffer == false)
		return false;

	// Early out if replay buffer disabled
	obs_output_t *replay_output = obs_frontend_get_replay_buffer_output();
	if (replay_output == nullptr)
		return false;
	obs_output_release(replay_output);

	return obs_hadowplay_has_active_captures();
}

#pragma region Update Thread

bool obs_hadowplay_update_thread_running = false;
bool obs_hadowplay_update_thread_closed = false;

pthread_t update_thread;

void *obs_hadowplay_update(void *param)
{
	UNUSED_PARAMETER(param);

	char thread_name[64];
	snprintf(thread_name, 64, "%s update thread", PLUGIN_NAME);
	os_set_thread_name(thread_name);

	os_atomic_store_bool(&obs_hadowplay_update_thread_running, true);

	while (os_atomic_load_bool(&obs_hadowplay_update_thread_running) ==
	       true) {

		if (obs_hadowplay_refresh_automatic_replay_buffer() == true) {
			if (obs_hadowplay_start_automatic_replay_buffer() ==
			    true) {
				obs_log(LOG_INFO,
					"Started automatic replay buffer");
			}
		} else {
			if (obs_hadowplay_stop_automatic_replay_buffer() ==
			    true) {
				obs_log(LOG_INFO,
					"Stopped automatic replay buffer");
			}
		}

		os_sleep_ms(1000);
	}

	obs_hadowplay_replay_buffer_stop();

	os_atomic_store_bool(&obs_hadowplay_update_thread_closed, true);

	return 0;
}

void obs_hadowplay_initialise_update_thread()
{
	int result = pthread_create(&update_thread, NULL, obs_hadowplay_update,
				    NULL);
	if (result != 0) {
		obs_log(LOG_ERROR,
			"Failed to create update thread (code %d), plugin is no longer able to track when to toggle the replay buffer",
			result);
	}
}

bool obs_hadowplay_close_update_thread()
{
	if (os_atomic_load_bool(&obs_hadowplay_update_thread_closed) == true) {
		return true;
	}

	os_atomic_store_bool(&obs_hadowplay_update_thread_running, false);

	obs_log(LOG_INFO, "Awaiting update thread closure");
	void *return_val = NULL;
	int result = pthread_join(update_thread, &return_val);
	if (result == 0) {
		obs_log(LOG_INFO, "Update thread closed");
	} else {
		obs_log(LOG_ERROR, "Failed to join update thread: %d", result);
		return false;
	}

	return true;
}

#pragma endregion

#pragma region Hooked/UnHooked signals

void obs_hadowplay_win_capture_hooked(void *data, calldata_t *calldata)
{
	UNUSED_PARAMETER(data);

	if (calldata == nullptr)
		return;

	obs_source_t *source = (obs_source_t *)calldata_ptr(calldata, "source");
	const char *title = calldata_string(calldata, "title");
	const char *win_class = calldata_string(calldata, "class");
	const char *exe = calldata_string(calldata, "executable");

	if (exe == nullptr)
		return;

	// TODO
	UNUSED_PARAMETER(source);
	UNUSED_PARAMETER(title);
	UNUSED_PARAMETER(win_class);
}

void obs_hadowplay_win_capture_unhooked(void *data, calldata_t *calldata)
{
	UNUSED_PARAMETER(data);
	
	if (calldata == nullptr)
		return;

	obs_source_t *source = (obs_source_t *)calldata_ptr(calldata, "source");

	// TODO
	UNUSED_PARAMETER(source);
}

#pragma endregion

#pragma region Activated/Deactivated signals

void obs_hadowplay_source_activated(void *data, calldata_t *calldata)
{
	UNUSED_PARAMETER(data);

	if (calldata == nullptr)
		return;

	obs_source_t *source = (obs_source_t *)calldata_ptr(calldata, "source");

	if (obs_hadowplay_is_capture_source(source)) {
		signal_handler_t *source_signal_handler =
			obs_source_get_signal_handler(source);
		signal_handler_connect(source_signal_handler, "hooked",
				       &obs_hadowplay_win_capture_hooked,
				       nullptr);
		signal_handler_connect(source_signal_handler, "unhooked",
				       &obs_hadowplay_win_capture_unhooked,
				       nullptr);
	}
}

void obs_hadowplay_source_deactivated(void *data, calldata_t *calldata)
{
	UNUSED_PARAMETER(data);

	if (calldata == nullptr)
		return;

	obs_source_t *source = (obs_source_t *)calldata_ptr(calldata, "source");

	if (obs_hadowplay_is_capture_source(source)) {
		signal_handler_t *source_signal_handler =
			obs_source_get_signal_handler(source);
		signal_handler_disconnect(source_signal_handler, "hooked",
					  &obs_hadowplay_win_capture_hooked,
					  nullptr);
		signal_handler_disconnect(source_signal_handler, "unhooked",
					  &obs_hadowplay_win_capture_unhooked,
					  nullptr);
	}
}

#pragma endregion

void obs_hadowplay_initialise()
{
	signal_handler_t *signal_handler = obs_get_signal_handler();
	signal_handler_connect(signal_handler, "source_activate",
			       &obs_hadowplay_source_activated, nullptr);
	signal_handler_connect(signal_handler, "source_deactivate",
			       &obs_hadowplay_source_deactivated, nullptr);
}

void obs_hadowplay_default_replay_buffer_event_callback(
	enum obs_frontend_event event, void *private_data)
{
	UNUSED_PARAMETER(private_data);

	switch (event) {

#pragma region Replay events

	case OBS_FRONTEND_EVENT_REPLAY_BUFFER_STOPPED: {
		if (os_atomic_load_bool(&obs_hadowplay_is_replay_controlled) ==
		    true) {
			obs_log(LOG_INFO, "Replay buffer manually stopped");
			os_atomic_store_bool(&obs_hadowplay_manual_stop, true);
		}

		os_atomic_store_bool(&obs_hadowplay_is_replay_controlled,
				     false);
		os_atomic_store_bool(&obs_hadowplay_manual_start, false);
		break;
	}

	case OBS_FRONTEND_EVENT_REPLAY_BUFFER_STARTED: {
		if (os_atomic_load_bool(&obs_hadowplay_is_replay_controlled) ==
		    false) {
			os_atomic_store_bool(&obs_hadowplay_manual_start, true);
		}
		break;
	}

	case OBS_FRONTEND_EVENT_REPLAY_BUFFER_SAVED: {
		const char *replay_path_c = obs_frontend_get_last_replay();

		if (replay_path_c == NULL) {
			return;
		}

		std::string target_name;
		if (Config::Inst().m_enable_auto_organisation == true &&
		    obs_hadowplay_get_captured_name(target_name) == true) {

			std::string new_filepath =
				obs_hadowplay_move_output_file(replay_path_c,
							       target_name);

			replay_path_c = new_filepath.c_str();
		}

		obs_hadowplay_notify_saved("Replay Saved", replay_path_c);

		if (Config::Inst().m_restart_replay_buffer_on_save == true) {
			obs_hadowplay_restart_replay_buffer();
		}
		break;
	}

#pragma endregion
	}
}

std::string recording_target_name;

void obs_hadowplay_frontend_event_callback(enum obs_frontend_event event,
					   void *private_data)
{
	UNUSED_PARAMETER(private_data);

	switch (event) {
	case OBS_FRONTEND_EVENT_FINISHED_LOADING: {
		obs_hadowplay_initialise_update_thread();
		break;
	}

#pragma region Screenshot event

	case OBS_FRONTEND_EVENT_SCREENSHOT_TAKEN: {
		const char *screenshot_path_c =
			obs_frontend_get_last_screenshot();

		if (screenshot_path_c == NULL) {
			return;
		}

		if (Config::Inst().m_enable_auto_organisation == true &&
		    Config::Inst().m_include_screenshots == true) {

			std::string target_name;
			if (obs_hadowplay_get_captured_name(target_name) ==
			    true) {

				std::string new_filepath =
					obs_hadowplay_move_output_file(
						screenshot_path_c, target_name);

				screenshot_path_c = new_filepath.c_str();
			}
		}

		obs_hadowplay_notify_saved("Screenshot Saved",
					   screenshot_path_c);
		break;
	}

#pragma endregion

#pragma region Recording events
	case OBS_FRONTEND_EVENT_RECORDING_STARTED: {
		// Reset recording name for fresh recordings
		recording_target_name.clear();
		std::string target_name;

		if (obs_hadowplay_get_captured_name(target_name) == true) {
			recording_target_name = target_name;
			obs_log(LOG_INFO, "Recording target found: %s",
				recording_target_name.c_str());
		}
		break;
	}

	case OBS_FRONTEND_EVENT_RECORDING_STOPPED: {
		const char *recording_path_c =
			obs_frontend_get_last_recording();

		if (recording_path_c == NULL) {
			return;
		}

		if (Config::Inst().m_enable_auto_organisation == true) {

			if (recording_target_name.empty() == true) {
				std::string target_name;

				if (obs_hadowplay_get_captured_name(
					    target_name) == true) {
					recording_target_name = target_name;
					obs_log(LOG_INFO,
						"Recording target found: %s",
						recording_target_name.c_str());
				}
			}

			if (recording_target_name.empty() == false) {

				std::string new_filepath =
					obs_hadowplay_move_output_file(
						recording_path_c,
						recording_target_name);

				recording_path_c = new_filepath.c_str();
			}
		}

		obs_hadowplay_notify_saved("Recording Saved", recording_path_c);

		break;
	}
#pragma endregion

	case OBS_FRONTEND_EVENT_EXIT: {
		obs_hadowplay_close_update_thread();
		break;
	}

	default: {
		break;
	}
	}
}

void obs_hadowplay_save_callback(obs_data_t *save_data, bool saving,
				 void *private_data)
{
	UNUSED_PARAMETER(private_data);

	if (saving == true) {
		Config::Inst().Save(save_data);
	} else {
		Config::Inst().Load(save_data);
	}
}

#include <QMainWindow>
#include "ui/SettingsDialog.hpp"

static SettingsDialog *settings_dialog = nullptr;

void obs_hadowplay_show_settings_dialog(void *data)
{
	UNUSED_PARAMETER(data);

	settings_dialog->show();
}

bool obs_module_load(void)
{
	obs_hadowplay_initialise();

	obs_frontend_add_event_callback(obs_hadowplay_frontend_event_callback,
					NULL);

	obs_frontend_add_event_callback(
		obs_hadowplay_default_replay_buffer_event_callback, NULL);

	obs_frontend_push_ui_translation(obs_module_get_string);

	settings_dialog = new SettingsDialog();
	settings_dialog->hide();

	obs_frontend_pop_ui_translation();

	obs_frontend_add_tools_menu_item(
		TEXT_SETTINGS_MENU, obs_hadowplay_show_settings_dialog, NULL);

	obs_frontend_add_save_callback(obs_hadowplay_save_callback, NULL);

	obs_log(LOG_INFO, "plugin loaded successfully (version %s)",
		PLUGIN_VERSION);

	return true;
}

void obs_module_unload()
{
	// Make sure the update thread has closed
	obs_hadowplay_close_update_thread();

	obs_log(LOG_INFO, "plugin unloaded");
}
