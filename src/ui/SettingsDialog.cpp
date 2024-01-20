#include "SettingsDialog.hpp"
#include "ui_SettingsDialog.h"

#include <obs-frontend-api.h>
#include <obs-module.h>
#include <QMainWindow>

#include <util/config-file.h>
#include "plugin-support.h"
#include "config/config.h"

static SettingsDialog *settings_dialog = nullptr;

void obs_hadowplay_qt_create_settings_dialog()
{
	QMainWindow *main_window =
		static_cast<QMainWindow *>(obs_frontend_get_main_window());

	settings_dialog = new SettingsDialog(main_window);
	settings_dialog->hide();
}

void obs_hadowplay_qt_show_settings_dialog()
{
	if (settings_dialog != nullptr) {
		settings_dialog->show();
	}
}

SettingsDialog::SettingsDialog(QWidget *parent)
	: QDialog(parent),
	  ui(new Ui::SettingsDialog)
{
	ui->setupUi(this);

	// Remove the ? button on dialogs on Windows
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

	connect(ui->button_box, &QDialogButtonBox::accepted, this,
		&SettingsDialog::button_box_accepted);
	connect(ui->button_box, &QDialogButtonBox::rejected, this,
		&SettingsDialog::button_box_rejected);
}

SettingsDialog::~SettingsDialog()
{
	delete ui;
}

void SettingsDialog::showEvent(QShowEvent *event)
{
	UNUSED_PARAMETER(event);

	config_t *profile_config = obs_frontend_get_profile_config();

	ui->automatic_replay_checkbox->setChecked(config_get_bool(
		profile_config, PLUGIN_NAME, CONFIG_AUTOREPLAY_ENABLED));
}

extern "C" void obs_hadowplay_replay_buffer_stop();
void SettingsDialog::ApplyConfig(void *data)
{
	auto *config = static_cast<config_t *>(data);

	config_set_bool(
		config, PLUGIN_NAME, CONFIG_AUTOREPLAY_ENABLED,
		settings_dialog->ui->automatic_replay_checkbox->isChecked());

	int result = config_save(config);
	if (result != 0) {
		blog(LOG_ERROR, "Failed to save config");
		return;
	}

	obs_hadowplay_replay_buffer_stop();
}

void SettingsDialog::button_box_accepted()
{
	config_t *profile_config = obs_frontend_get_profile_config();

	settings_dialog->ApplyConfig(profile_config);
}

void SettingsDialog::button_box_rejected() {}
