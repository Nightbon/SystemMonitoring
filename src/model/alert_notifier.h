#pragma once
#include <string>
#include <vector>

#include "email_sender.h"
#include "telegram_bot.h"

namespace s21 {

class AlertNotifier {
 public:
  void SetTelegramCredentials(const std::string& token,
                              const std::string& chat_id,
                              const std::string& proxy);
  void SetEmailCredentials(const std::string& smtp_server, int smtp_port,
                           const std::string& username,
                           const std::string& password,
                           const std::string& from_address);
  void SetEmailRecipient(const std::string& to_address);
  void SetEmailEnabled(bool enabled);
  void SetTelegramCurlTimeout(long timeout_sec);
  void SetEmailCurlTimeout(long timeout_sec);

  void ProcessAlerts(const std::vector<std::string>& alerts);

 private:
  TelegramBot telegram_bot_;
  EmailSender email_sender_;
};

}  // namespace s21
