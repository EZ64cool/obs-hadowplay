#include "plugin-platform-helpers.hpp"

bool dstr_get_filename(struct dstr *filepath, struct dstr *filename)
{
	const char *filename_start = strrchr(filepath->array, '\\') + 1;

	const char *filename_end = strrchr(filename_start, '.');

	if (filename_start != NULL && filename_end != NULL) {
		size_t start = (filename_start - filepath->array);
		size_t count = (filename_end - filename_start);

		dstr_mid(filename, filepath, start, count);
	} else {
		return false;
	}

	return true;
}

#include <QString>
#include <obs-module.h>

extern "C" void obs_hadowplay_play_sound(const wchar_t *filepath);

void obs_hadowplay_play_notif_sound()
{
	QString filepath = obs_module_file("notification.wav");

	obs_hadowplay_play_sound(filepath.toStdWString().c_str());
}

#include <QSystemTrayIcon>
#include <obs-frontend-api.h>

struct SystemTrayNotification {
	QString title;
	QString message;
};

bool obs_hadowplay_show_notification(struct dstr *title, struct dstr *message)
{
	if (!QSystemTrayIcon::isSystemTrayAvailable() ||
	    !QSystemTrayIcon::supportsMessages())
		return false;

	SystemTrayNotification *notification = new SystemTrayNotification{
		QString(title->array), QString(message->array)};

	obs_queue_task(
		OBS_TASK_UI,
		[](void *param) {
			void *systemTrayPtr = obs_frontend_get_system_tray();
			auto systemTray =
				static_cast<QSystemTrayIcon *>(systemTrayPtr);

			auto notification =
				static_cast<SystemTrayNotification *>(param);
			systemTray->showMessage(notification->title,
						notification->message,
						QSystemTrayIcon::NoIcon);
			delete notification;
		},
		(void *)notification, false);

	return true;
}