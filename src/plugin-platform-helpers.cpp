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
#include <QObject>
#include <QFileInfo>
#include <QDir>
#include <QUrl>
#include <QDesktopServices>
#include <obs-frontend-api.h>

#ifdef _WIN32
extern void obs_hadowplay_win_open_file_location(const wchar_t *path);
#endif

// Path of the last file we showed a notification for, opened when it is clicked
static QString obs_hadowplay_last_notified_path;

static void obs_hadowplay_open_file_location(const QString &path)
{
	if (path.isEmpty())
		return;

	QFileInfo info(path);

#ifdef _WIN32
	if (info.exists()) {
		obs_hadowplay_win_open_file_location(
			QDir::toNativeSeparators(info.absoluteFilePath())
				.toStdWString()
				.c_str());
		return;
	}
#endif

	QString folder = info.exists() ? info.absolutePath() : info.path();
	if (QFileInfo(folder).isDir())
		QDesktopServices::openUrl(QUrl::fromLocalFile(folder));
}

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

	SystemTrayNotification *notification =
		new SystemTrayNotification{QString::fromUtf8(title.c_str()),
					   QString::fromUtf8(message.c_str())};

	obs_queue_task(
		OBS_TASK_UI,
		[](void *param) {
			void *systemTrayPtr = obs_frontend_get_system_tray();
			auto systemTray =
				static_cast<QSystemTrayIcon *>(systemTrayPtr);

			auto notification =
				static_cast<SystemTrayNotification *>(param);

			if (systemTray == nullptr) {
				delete notification;
				return;
			}

			static bool click_connected = false;
			if (click_connected == false) {
				QObject::connect(
					systemTray,
					&QSystemTrayIcon::messageClicked, [] {
						obs_hadowplay_open_file_location(
							obs_hadowplay_last_notified_path);
					});
				click_connected = true;
			}

			obs_hadowplay_last_notified_path =
				notification->message;

			systemTray->showMessage(notification->title,
						notification->message,
						QSystemTrayIcon::NoIcon);
			delete notification;
		},
		(void *)notification, false);

	return true;
}