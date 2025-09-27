#include "plugin-platform-helpers.hpp"
#include "config/config.hpp"

#include <QString>
#include <obs-module.h>
#include <util/platform.h>

#if !defined(_WIN32) && !defined(_WIN64)
#include <strings.h>
#define strcmpi strcasecmp
#endif

void obs_hadowplay_play_notif_sound()
{
	QString filepath = obs_module_file("notification.wav");

#if defined(_WIN32) || defined(_WIN64) || defined(__WINDOWS__) || \
	defined(__TOS_WIN__)
	obs_hadowplay_play_sound(filepath.toStdWString().c_str());
#else
	obs_hadowplay_play_sound(filepath.toStdString().c_str());
#endif
}

std::string
obs_hadowplay_strip_executable_extension(const std::string &filename)
{
	const char *ext = os_get_path_extension(filename.c_str());
	if (ext != nullptr && strcmpi(ext, ".exe") == 0) {
		return filename.substr(0, ext - filename.c_str());
	}
	return filename;
}

bool obs_hadowplay_is_exe_excluded(const char *exe)
{
	if (exe == nullptr)
		return true;

	std::string exe_str = obs_hadowplay_strip_executable_extension(exe);

	for (const std::string &val : Config::Inst().m_exclusions) {
		if (strcmpi(val.c_str(), exe_str.c_str()) == 0) {
			return true;
		}
	}

	return false;
}

#include <QSystemTrayIcon>
#include <obs-frontend-api.h>

struct SystemTrayNotification {
	QString title;
	QString message;
};

bool obs_hadowplay_show_notification(const std::string &title,
				     const std::string &message)
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

			auto notification_data =
				static_cast<SystemTrayNotification *>(param);
			systemTray->showMessage(notification_data->title,
						notification_data->message,
						QSystemTrayIcon::NoIcon);
			delete notification_data;
		},
		(void *)notification, false);

	return true;
}