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

	void ApplyConfig(void *data);
	void showEvent(QShowEvent *event);

private slots:
	void button_box_accepted();
	void button_box_rejected();

private:
	Ui::SettingsDialog *ui;
};
#else
#define EXTERNC
#endif

EXTERNC void obs_hadowplay_qt_create_settings_dialog();
EXTERNC void obs_hadowplay_qt_show_settings_dialog();

#endif // SETTINGSDIALOG_H
