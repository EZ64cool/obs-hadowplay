#pragma once

#define CONFIG_AUTOREPLAY_ENABLED "AutoReplayEnabled"
#define CONFIG_EXCLUSION_ITEM_STRING "AppName"
#define CONFIG_EXCLUSIONS "Exclusions"

#include <string>
#include <vector>
#include <atomic>
#include <obs.h>

struct Config {

	static Config &Inst();

	Config();

	void Load(obs_data_t *load_data);
	void Save(obs_data_t *save_data);

	void SetDefaults(obs_data_t *data);

	bool m_auto_replay_buffer = false;
	std::vector<std::string> m_exclusions = std::vector<std::string>();
};