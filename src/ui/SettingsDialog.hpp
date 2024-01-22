#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#ifdef __cplusplus
#define EXTERNC extern "C"

#include <QDialog>

struct Config;

namespace Ui {
class SettingsDialog;
}

class SettingsDialog : public QDialog {
	Q_OBJECT

public:
	explicit SettingsDialog(Config &config, QWidget *parent = nullptr);
	~SettingsDialog();

	void ApplyConfig();
	void showEvent(QShowEvent *event);

private slots:
	void button_box_accepted();

private:
	Ui::SettingsDialog *ui;
	Config &m_config;
};
#else
#define EXTERNC
#endif

#endif // SETTINGSDIALOG_H
