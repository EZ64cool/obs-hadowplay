#include "config.hpp"
#include "plugin-support.h"
#include <obs.hpp>

Config::Config() {}

void Config::Save(obs_data_t *save_data)
{
	OBSDataAutoRelease hadowplay_data = obs_data_create();

	obs_data_set_bool(hadowplay_data, CONFIG_AUTOREPLAY_ENABLED,
			  this->m_auto_replay_buffer);

	OBSDataArrayAutoRelease exclusions = obs_data_array_create();

	for (size_t i = 0; i < this->m_exclusions.size(); ++i) {
		OBSDataAutoRelease item = obs_data_create();
		obs_data_set_string(item, CONFIG_EXCLUSION_ITEM_STRING,
				    this->m_exclusions[i].c_str());
		obs_data_array_push_back(exclusions, item);
	}

	obs_data_set_obj(save_data, PLUGIN_NAME, hadowplay_data);
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
}

static const char *default_exclusions[] = {"vlc", "plex", NULL};

void Config::SetDefaults(obs_data_t *hadowplay_data)
{
	obs_data_set_default_bool(hadowplay_data, CONFIG_AUTOREPLAY_ENABLED,
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
