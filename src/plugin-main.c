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

#include "plugin-macros.generated.h"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

void obs_hadowplay_consume_enum_source(obs_source_t *parent,
				       obs_source_t *source, void *param)
{
	UNUSED_PARAMETER(parent);

	bool *found = (bool *)param;

	if (*found == true)
		return;

	const char *id = obs_source_get_id(source);

	if (strcmp(id, "game_capture") == 0) {

		const char *source_name = obs_source_get_name(source);

		blog(LOG_INFO, "Game capture found: %s", source_name);

		uint32_t width = obs_source_get_width(source);
		uint32_t height = obs_source_get_height(source);

		*found = width > 0 && height > 0;

		bool activate_replay = *found &&
				       !obs_frontend_replay_buffer_active();

		bool deactivate_replay = width == 0 && height == 0 &&
					 obs_frontend_replay_buffer_active();

		if (activate_replay) {
			obs_frontend_replay_buffer_start();
		} else if (deactivate_replay) {
			obs_frontend_replay_buffer_stop();
		}
	}
}

void *obs_hadowplay_update(void *param)
{
	UNUSED_PARAMETER(param);

	while (1) {
		blog(LOG_INFO, "Update");

		obs_source_t *scene_source = obs_frontend_get_current_scene();

		bool found = false;

		obs_source_enum_active_sources(
			scene_source, obs_hadowplay_consume_enum_source,
			&found);

		obs_source_release(scene_source);

		os_sleep_ms(1000);
	}

	return 0;
}

void obs_hadowplay_frontend_event_callback(enum obs_frontend_event event,
					   void *private_data)
{
	UNUSED_PARAMETER(private_data);

	if (event == OBS_FRONTEND_EVENT_FINISHED_LOADING) {

		blog(LOG_INFO, "Frontend finished loading");

		pthread_t *update_thread = NULL;

		pthread_create(update_thread, NULL, obs_hadowplay_update, NULL);
	}
}

bool obs_module_load(void)
{
	blog(LOG_INFO, "plugin loaded successfully (version %s)",
	     PLUGIN_VERSION);

	obs_frontend_add_event_callback(obs_hadowplay_frontend_event_callback,
					NULL);

	return true;
}

void obs_module_unload()
{
	blog(LOG_INFO, "plugin unloaded");
}