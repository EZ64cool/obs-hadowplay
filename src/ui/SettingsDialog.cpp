#include "SettingsDialog.hpp"
#include "ui_SettingsDialog.h"

#include <obs-frontend-api.h>
#include <obs-module.h>
#include <QMainWindow>

static SettingsDialog *settings_dialog = nullptr;

void obs_hadowplay_qt_create_settings_dialog()
{
	QMainWindow *main_window =
		static_cast<QMainWindow *>(obs_frontend_get_main_window());

	settings_dialog = new SettingsDialog(main_window);
	settings_dialog->hide();
}

void obs_hadowplay_qt_destroy_settings_dialog()
{
	if (settings_dialog != nullptr) {
		delete settings_dialog;
		settings_dialog = nullptr;
	}
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
}

SettingsDialog::~SettingsDialog()
{
	delete ui;
}
