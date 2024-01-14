#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#ifdef __cplusplus
#define EXTERNC extern "C"

#include <QDialog>

namespace Ui {
class SettingsDialog;
}

class SettingsDialog : public QDialog {
	Q_OBJECT

public:
	explicit SettingsDialog(QWidget *parent = nullptr);
	~SettingsDialog();

private:
	Ui::SettingsDialog *ui;
};
#else
#define EXTERNC
#endif

EXTERNC void obs_hadowplay_qt_create_settings_dialog();
EXTERNC void obs_hadowplay_qt_destroy_settings_dialog();
EXTERNC void obs_hadowplay_qt_show_settings_dialog();

#endif // SETTINGSDIALOG_H
