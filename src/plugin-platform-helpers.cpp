#include "plugin-platform-helpers.hpp"

#include <QString>
#include <obs-module.h>

extern void obs_hadowplay_play_sound(const wchar_t *filepath);

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

bool obs_hadowplay_show_notification(std::string &title, std::string &message)
{
	if (!QSystemTrayIcon::isSystemTrayAvailable() ||
	    !QSystemTrayIcon::supportsMessages())
		return false;

	SystemTrayNotification *notification = new SystemTrayNotification{
		QString(title.c_str()), QString(message.c_str())};

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