#pragma once
#include <string>
#include <vector>

namespace s21 {

class TelegramBot {
 public:
  TelegramBot() = default;
  explicit TelegramBot(std::string token, std::string chat_id,
                       std::string proxy = {});

  void SetCredentials(std::string token, std::string chat_id,
                      std::string proxy = {});
  void SetCurlTimeout(long timeout_sec);
  [[nodiscard]] bool IsConfigured() const;
  bool SendAlert(const std::string& alert);

 private:
  std::string token_;
  std::string chat_id_;
  std::string proxy_;
  long curl_timeout_sec_ = 10;
};

}  // namespace s21
