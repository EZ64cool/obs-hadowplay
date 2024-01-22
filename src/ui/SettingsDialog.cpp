#include "SettingsDialog.hpp"
#include "ui_SettingsDialog.h"

#include <obs-frontend-api.h>
#include <obs-module.h>

#include <util/config-file.h>
#include "plugin-support.h"
#include "config/config.hpp"

SettingsDialog::SettingsDialog(Config &config, QWidget *parent)
	: QDialog(parent),
	  ui(new Ui::SettingsDialog),
	  m_config(config)
{
	ui->setupUi(this);

	// Remove the ? button on dialogs on Windows
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

	connect(ui->button_box, &QDialogButtonBox::accepted, this,
		&SettingsDialog::button_box_accepted);
}

SettingsDialog::~SettingsDialog()
{
	delete ui;
}

void SettingsDialog::showEvent(QShowEvent *event)
{
	UNUSED_PARAMETER(event);

	ui->automatic_replay_checkbox->setChecked(
		m_config.m_auto_replay_buffer);

	ui->exceptions_list->clear();
	for (int i = 0; i < this->m_config.m_exclusions.size(); ++i) {
		this->ui->exceptions_list->addItem(
			QString::fromStdString(this->m_config.m_exclusions[i]));
	}
}

extern void obs_hadowplay_replay_buffer_stop();
void SettingsDialog::ApplyConfig()
{
	this->m_config.m_auto_replay_buffer =
		this->ui->automatic_replay_checkbox->isChecked();

	int count = this->ui->exceptions_list->count();

	this->m_config.m_exclusions.clear();
	for (int i = 0; i < count; ++i) {
		auto item = this->ui->exceptions_list->item(i);
		this->m_config.m_exclusions.push_back(
			item->text().toStdString().c_str());
	}

	obs_hadowplay_replay_buffer_stop();
}

void SettingsDialog::button_box_accepted()
{
	this->ApplyConfig();
}
