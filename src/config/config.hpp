#pragma once

#define CONFIG_AUTOREPLAY_ENABLED "AutoReplayEnabled"
#define CONFIG_AUTOREPLY_BUFFER_STOP_DELAY "AutoReplayBufferStopDelay"
#define CONFIG_AUTOREPLY_RESET_ON_SAVE "AutoReplayResetOnSave"
#define CONFIG_PLAY_NOTIF_SOUND "PlayNotifSound"
#define CONFIG_SHOW_DESKTOP_NOTIF "ShowDesktopNotif"
#define CONFIG_ENABLE_AUTO_ORGANISATION "EnableAutoOrganisation"
#define CONFIG_USE_CUSTOM_FILENAME_FORMAT "UseCustomFilenameFormat"
#define CONFIG_CUSTOM_FILENAME_ARRANGEMENT "CustomFilenameArrangment"
#define CONFIG_CUSTOM_FILENAME_SEPERATOR "CustomFilenameSeperator"
#define CONFIG_INCLUDE_SCREENSHOTS "IncludeScreenshots"
#define CONFIG_EXCLUSION_ITEM_STRING "AppName"
#define CONFIG_EXCLUSIONS "Exclusions"

#pragma region BackwardsCompatability
#define CONFIG_FOLDER_NAME_AS_PREFIX "FolderNameAsPrefix"
#pragma endregion

#include <string>
#include <vector>
#include <atomic>
#include <obs.h>

enum FilenameArrangement { TargetBefore = 0, TargetAfter = 1 };

struct Config {

	static Config &Inst();

	Config();

	void Load(obs_data_t *load_data);
	void Save(obs_data_t *save_data);

	void SetDefaults(obs_data_t *data);

private:
	void LoadBackwardsCompatability(obs_data_t *load_data);

public:
	std::vector<std::string> m_exclusions = std::vector<std::string>();

	// Auto replay buffer settings
	bool m_auto_replay_buffer = true;
	int m_auto_replay_buffer_stop_delay = 0;
	bool m_restart_replay_buffer_on_save = false;

	// Auto organisation settings
	bool m_enable_folder_organisation = true;
	bool m_include_screenshots = true;

	// Custom filename setting
	bool m_use_custom_filename_format = true;
	FilenameArrangement m_custom_filename_arrangement = TargetBefore;
	char m_custom_filename_seperator = '-';

	// Notification settings
	bool m_play_notif_sound = true;
	bool m_show_desktop_notif = true;
};
