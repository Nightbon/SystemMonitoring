#pragma once
#include <string>
#include <vector>

namespace s21 {

class EmailSender {
 public:
  EmailSender() = default;
  explicit EmailSender(std::string smtp_server, int smtp_port,
                       std::string username, std::string password,
                       std::string from_address);

  void SetCredentials(std::string smtp_server, int smtp_port,
                      std::string username, std::string password,
                      std::string from_address);
  void SetRecipient(std::string to_address);
  void SetEnabled(bool enabled);
  void SetCurlTimeout(long timeout_sec);

  [[nodiscard]] bool IsConfigured() const;
  [[nodiscard]] bool IsEnabled() const;
  bool SendAlert(const std::string& alert);

 private:
  std::string smtp_server_;
  int smtp_port_ = 587;
  std::string username_;
  std::string password_;
  std::string from_address_;
  std::string to_address_;
  bool enabled_ = false;
  long curl_timeout_sec_ = 10;
};

}  // namespace s21
