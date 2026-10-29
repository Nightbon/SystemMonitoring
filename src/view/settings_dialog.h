#pragma once
#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QSpinBox>

namespace s21 {

struct NotificationSettings {
  // Telegram
  std::string telegram_token;
  std::string telegram_chat_id;
  std::string proxy;

  // Email
  std::string smtp_server;
  int smtp_port = 587;
  std::string email_username;
  std::string email_password;
  std::string email_from;
  std::string email_to;
  bool email_enabled = false;
};

class SettingsDialog : public QDialog {
  Q_OBJECT

 public:
  explicit SettingsDialog(const NotificationSettings& settings,
                          QWidget* parent = nullptr);
  [[nodiscard]] NotificationSettings GetSettings() const;

 private:
  void SetupUi();

  QLineEdit* telegram_token_edit_;
  QLineEdit* telegram_chat_id_edit_;
  QLineEdit* proxy_edit_;
  QLineEdit* smtp_server_edit_;
  QSpinBox* smtp_port_spin_;
  QLineEdit* email_username_edit_;
  QLineEdit* email_password_edit_;
  QLineEdit* email_from_edit_;
  QLineEdit* email_to_edit_;
  QCheckBox* email_enabled_check_;
};

}  // namespace s21
