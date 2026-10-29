#include "alert_notifier.h"

#include <format>

namespace s21 {

void AlertNotifier::SetTelegramCredentials(const std::string& token,
                                           const std::string& chat_id,
                                           const std::string& proxy) {
  telegram_bot_.SetCredentials(token, chat_id, proxy);
}

void AlertNotifier::SetEmailCredentials(const std::string& smtp_server,
                                        int smtp_port,
                                        const std::string& username,
                                        const std::string& password,
                                        const std::string& from_address) {
  email_sender_.SetCredentials(smtp_server, smtp_port, username, password,
                               from_address);
}

void AlertNotifier::SetEmailRecipient(const std::string& to_address) {
  email_sender_.SetRecipient(to_address);
}

void AlertNotifier::SetEmailEnabled(bool enabled) {
  email_sender_.SetEnabled(enabled);
}

void AlertNotifier::SetTelegramCurlTimeout(long timeout_sec) {
  telegram_bot_.SetCurlTimeout(timeout_sec);
}

void AlertNotifier::SetEmailCurlTimeout(long timeout_sec) {
  email_sender_.SetCurlTimeout(timeout_sec);
}

void AlertNotifier::ProcessAlerts(const std::vector<std::string>& alerts) {
  for (const auto& alert : alerts) {
    telegram_bot_.SendAlert(alert);
    email_sender_.SendAlert(alert);
  }
}

}  // namespace s21
