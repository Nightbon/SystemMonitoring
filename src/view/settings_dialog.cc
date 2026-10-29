#include "settings_dialog.h"

#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QVBoxLayout>

namespace s21 {

SettingsDialog::SettingsDialog(const NotificationSettings& settings,
                               QWidget* parent)
    : QDialog(parent) {
  setWindowTitle("Notification Settings");
  SetupUi();

  telegram_token_edit_->setText(
      QString::fromStdString(settings.telegram_token));
  telegram_chat_id_edit_->setText(
      QString::fromStdString(settings.telegram_chat_id));
  proxy_edit_->setText(QString::fromStdString(settings.proxy));
  smtp_server_edit_->setText(QString::fromStdString(settings.smtp_server));
  smtp_port_spin_->setValue(settings.smtp_port);
  email_username_edit_->setText(
      QString::fromStdString(settings.email_username));
  email_password_edit_->setText(
      QString::fromStdString(settings.email_password));
  email_from_edit_->setText(QString::fromStdString(settings.email_from));
  email_to_edit_->setText(QString::fromStdString(settings.email_to));
  email_enabled_check_->setChecked(settings.email_enabled);
}

void SettingsDialog::SetupUi() {
  auto* main_layout = new QVBoxLayout(this);

  // Telegram group
  auto* tg_group = new QGroupBox("Telegram Bot", this);
  auto* tg_layout = new QFormLayout(tg_group);
  telegram_token_edit_ = new QLineEdit(this);
  telegram_token_edit_->setEchoMode(QLineEdit::Password);
  tg_layout->addRow("Bot Token:", telegram_token_edit_);
  telegram_chat_id_edit_ = new QLineEdit(this);
  tg_layout->addRow("Chat ID:", telegram_chat_id_edit_);
  proxy_edit_ = new QLineEdit(this);
  tg_layout->addRow("Proxy:", proxy_edit_);
  main_layout->addWidget(tg_group);

  // Email group
  auto* email_group = new QGroupBox("Email Notifications", this);
  auto* email_layout = new QFormLayout(email_group);
  email_enabled_check_ = new QCheckBox("Enable email notifications", this);
  email_layout->addRow(email_enabled_check_);
  smtp_server_edit_ = new QLineEdit(this);
  email_layout->addRow("SMTP Server:", smtp_server_edit_);
  smtp_port_spin_ = new QSpinBox(this);
  smtp_port_spin_->setRange(1, 65535);
  smtp_port_spin_->setValue(587);
  email_layout->addRow("SMTP Port:", smtp_port_spin_);
  email_username_edit_ = new QLineEdit(this);
  email_layout->addRow("Username:", email_username_edit_);
  email_password_edit_ = new QLineEdit(this);
  email_password_edit_->setEchoMode(QLineEdit::Password);
  email_layout->addRow("Password:", email_password_edit_);
  email_from_edit_ = new QLineEdit(this);
  email_layout->addRow("From Address:", email_from_edit_);
  email_to_edit_ = new QLineEdit(this);
  email_layout->addRow("To Address:", email_to_edit_);
  main_layout->addWidget(email_group);

  auto* buttons = new QDialogButtonBox(
      QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
  main_layout->addWidget(buttons);
  connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

NotificationSettings SettingsDialog::GetSettings() const {
  NotificationSettings s;
  s.telegram_token = telegram_token_edit_->text().toStdString();
  s.telegram_chat_id = telegram_chat_id_edit_->text().toStdString();
  s.proxy = proxy_edit_->text().toStdString();
  s.smtp_server = smtp_server_edit_->text().toStdString();
  s.smtp_port = smtp_port_spin_->value();
  s.email_username = email_username_edit_->text().toStdString();
  s.email_password = email_password_edit_->text().toStdString();
  s.email_from = email_from_edit_->text().toStdString();
  s.email_to = email_to_edit_->text().toStdString();
  s.email_enabled = email_enabled_check_->isChecked();
  return s;
}

}  // namespace s21
