#include "config.hpp"
#include "plugin-support.h"
#include <obs.hpp>

static Config config;

Config &Config::Inst()
{
	return config;
}

Config::Config() {}

void Config::Save(obs_data_t *save_data)
{
	OBSDataAutoRelease hadowplay_data = obs_data_create();

	obs_data_set_bool(hadowplay_data, CONFIG_AUTOREPLAY_ENABLED,
			  this->m_auto_replay_buffer);

	obs_data_set_int(hadowplay_data, CONFIG_AUTOREPLY_BUFFER_STOP_DELAY,
			 this->m_auto_replay_buffer_stop_delay);

	obs_data_set_bool(hadowplay_data, CONFIG_AUTOREPLY_RESET_ON_SAVE,
			  this->m_restart_replay_buffer_on_save);

	obs_data_set_bool(hadowplay_data, CONFIG_ENABLE_AUTO_ORGANISATION,
			  this->m_enable_folder_organisation);

	obs_data_set_bool(hadowplay_data, CONFIG_USE_CUSTOM_FILENAME_FORMAT,
			  this->m_use_custom_filename_format);

	obs_data_set_int(hadowplay_data, CONFIG_CUSTOM_FILENAME_SEPERATOR,
			 this->m_custom_filename_seperator);

	obs_data_set_int(hadowplay_data, CONFIG_CUSTOM_FILENAME_ARRANGEMENT,
			 this->m_custom_filename_arrangement);

	obs_data_set_bool(hadowplay_data, CONFIG_INCLUDE_SCREENSHOTS,
			  this->m_include_screenshots);

	obs_data_set_bool(hadowplay_data, CONFIG_PLAY_NOTIF_SOUND,
			  this->m_play_notif_sound);

	obs_data_set_bool(hadowplay_data, CONFIG_SHOW_DESKTOP_NOTIF,
			  this->m_show_desktop_notif);

	OBSDataArrayAutoRelease exclusions = obs_data_array_create();

	for (size_t i = 0; i < this->m_exclusions.size(); ++i) {
		OBSDataAutoRelease item = obs_data_create();
		obs_data_set_string(item, CONFIG_EXCLUSION_ITEM_STRING,
				    this->m_exclusions[i].c_str());
		obs_data_array_push_back(exclusions, item);
	}

	obs_data_set_array(hadowplay_data, CONFIG_EXCLUSIONS, exclusions);

	obs_data_set_obj(save_data, PLUGIN_NAME, hadowplay_data);
}

void Config::LoadBackwardsCompatability(obs_data_t *hadowplay_data)
{
	bool folder_name_as_prefix =
		obs_data_get_bool(hadowplay_data, CONFIG_FOLDER_NAME_AS_PREFIX);

	if (folder_name_as_prefix) {
		this->m_use_custom_filename_format = true;
		this->m_custom_filename_seperator = '_';
		this->m_custom_filename_arrangement = TargetBefore;
	}
}

void Config::Load(obs_data_t *load_data)
{
	OBSDataAutoRelease hadowplay_data =
		obs_data_get_obj(load_data, PLUGIN_NAME);

	if (hadowplay_data == nullptr) {
		hadowplay_data = obs_data_create();
	}

	this->SetDefaults(hadowplay_data);

	this->m_auto_replay_buffer =
		obs_data_get_bool(hadowplay_data, CONFIG_AUTOREPLAY_ENABLED);

	this->m_auto_replay_buffer_stop_delay = (int)obs_data_get_int(
		hadowplay_data, CONFIG_AUTOREPLY_BUFFER_STOP_DELAY);

	this->m_restart_replay_buffer_on_save = obs_data_get_bool(
		hadowplay_data, CONFIG_AUTOREPLY_RESET_ON_SAVE);

	this->m_enable_folder_organisation = obs_data_get_bool(
		hadowplay_data, CONFIG_ENABLE_AUTO_ORGANISATION);

	this->m_use_custom_filename_format = obs_data_get_bool(
		hadowplay_data, CONFIG_USE_CUSTOM_FILENAME_FORMAT);

	this->m_custom_filename_seperator = (char)obs_data_get_int(
		hadowplay_data, CONFIG_CUSTOM_FILENAME_SEPERATOR);

	this->m_custom_filename_arrangement =
		(FilenameArrangement)obs_data_get_int(
			hadowplay_data, CONFIG_CUSTOM_FILENAME_ARRANGEMENT);

	this->m_include_screenshots =
		obs_data_get_bool(hadowplay_data, CONFIG_INCLUDE_SCREENSHOTS);

	this->m_play_notif_sound =
		obs_data_get_bool(hadowplay_data, CONFIG_PLAY_NOTIF_SOUND);

	this->m_show_desktop_notif =
		obs_data_get_bool(hadowplay_data, CONFIG_SHOW_DESKTOP_NOTIF);

	OBSDataArrayAutoRelease exclusions =
		obs_data_get_array(hadowplay_data, CONFIG_EXCLUSIONS);

	size_t length = obs_data_array_count(exclusions);

	this->m_exclusions.clear();
	for (size_t i = 0; i < length; ++i) {
		OBSDataAutoRelease item = obs_data_array_item(exclusions, i);
		const char *app_name =
			obs_data_get_string(item, CONFIG_EXCLUSION_ITEM_STRING);
		this->m_exclusions.push_back(std::string(app_name));
	}

	this->LoadBackwardsCompatability(hadowplay_data);
}

static const char *default_exclusions[] = {"vlc", "plex", NULL};

void Config::SetDefaults(obs_data_t *hadowplay_data)
{
	obs_data_set_default_bool(hadowplay_data, CONFIG_AUTOREPLAY_ENABLED,
				  true);

	obs_data_set_default_int(hadowplay_data,
				 CONFIG_AUTOREPLY_BUFFER_STOP_DELAY, 0);

	obs_data_set_default_bool(hadowplay_data,
				  CONFIG_AUTOREPLY_RESET_ON_SAVE, false);

	obs_data_set_default_bool(hadowplay_data,
				  CONFIG_ENABLE_AUTO_ORGANISATION, true);

	obs_data_set_default_bool(hadowplay_data,
				  CONFIG_USE_CUSTOM_FILENAME_FORMAT, true);

	obs_data_set_default_string(hadowplay_data,
				    CONFIG_CUSTOM_FILENAME_SEPERATOR, "-");

	obs_data_set_default_int(hadowplay_data,
				 CONFIG_CUSTOM_FILENAME_ARRANGEMENT,
				 TargetBefore);

	obs_data_set_default_bool(hadowplay_data, CONFIG_PLAY_NOTIF_SOUND,
				  true);

	obs_data_set_default_bool(hadowplay_data, CONFIG_SHOW_DESKTOP_NOTIF,
				  true);

	OBSDataArrayAutoRelease exclusion_array = obs_data_array_create();

	for (const char **vals = default_exclusions; *vals; vals++) {
		OBSDataAutoRelease item = obs_data_create();
		obs_data_set_string(item, CONFIG_EXCLUSION_ITEM_STRING, *vals);
		obs_data_array_push_back(exclusion_array, item);
	}

	obs_data_set_default_array(hadowplay_data, CONFIG_EXCLUSIONS,
				   exclusion_array);
}
