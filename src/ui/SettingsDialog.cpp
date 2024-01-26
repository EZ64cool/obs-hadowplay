#include "SettingsDialog.hpp"
#include "ui_SettingsDialog.h"

#include <obs-frontend-api.h>
#include <obs-module.h>

#include <util/config-file.h>
#include "plugin-support.h"
#include "config/config.hpp"

SettingsDialog::SettingsDialog() : QDialog(nullptr), ui(new Ui::SettingsDialog)
{
	ui->setupUi(this);

	// Remove the ? button on dialogs on Windows
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

	connect(ui->button_box, &QDialogButtonBox::accepted, this,
		&SettingsDialog::button_box_accepted);

	connect(ui->add_exception_button, &QPushButton::pressed, this,
		&SettingsDialog::add_exclusion_pressed);

	connect(ui->edit_exception_button, &QPushButton::pressed, this,
		&SettingsDialog::edit_exclusion_pressed);

	connect(ui->delete_exception_button, &QPushButton::pressed, this,
		&SettingsDialog::delete_exclusion_pressed);
}

SettingsDialog::~SettingsDialog()
{
	delete ui;
}

void SettingsDialog::showEvent(QShowEvent *event)
{
	UNUSED_PARAMETER(event);

	ui->automatic_replay_checkbox->setChecked(
		Config::Inst().m_auto_replay_buffer);

	ui->exceptions_list->clear();
	for (int i = 0; i < Config::Inst().m_exclusions.size(); ++i) {
		this->ui->exceptions_list->addItem(
			QString::fromStdString(Config::Inst().m_exclusions[i]));
	}
}

extern void obs_hadowplay_replay_buffer_stop();
void SettingsDialog::ApplyConfig()
{
	Config::Inst().m_auto_replay_buffer =
		this->ui->automatic_replay_checkbox->isChecked();

	int count = this->ui->exceptions_list->count();

	Config::Inst().m_exclusions.clear();
	for (int i = 0; i < count; ++i) {
		auto item = this->ui->exceptions_list->item(i);
		Config::Inst().m_exclusions.push_back(
			item->text().toStdString().c_str());
	}

	obs_hadowplay_replay_buffer_stop();
}

#include <QInputDialog>

void SettingsDialog::add_exclusion_pressed()
{
	bool ok;
	QString text = QInputDialog::getText(
		this, obs_module_text("OBS.Hadowplay.AddException"),
		"Process name", QLineEdit::Normal, QString(), &ok);

	if (ok && text.isEmpty() == false) {
		this->ui->exceptions_list->addItem(text);
	}
}

void SettingsDialog::edit_exclusion_pressed()
{
	bool ok;
	QString text = QInputDialog::getText(
		this, obs_module_text("OBS.Hadowplay.EditException"),
		"Process name", QLineEdit::Normal,
		this->ui->exceptions_list->currentItem()->text(), &ok);

	if (ok && text.isEmpty() == false) {
		this->ui->exceptions_list->currentItem()->setText(text);
	}
}

void SettingsDialog::delete_exclusion_pressed()
{
	qDeleteAll(this->ui->exceptions_list->selectedItems());
}

void SettingsDialog::button_box_accepted()
{
	this->ApplyConfig();
}
