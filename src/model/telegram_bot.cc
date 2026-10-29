#include "telegram_bot.h"

#include <curl/curl.h>

#include <cstdio>
#include <format>
#include <memory>

namespace s21 {

TelegramBot::TelegramBot(std::string token, std::string chat_id,
                         std::string proxy)
    : token_(std::move(token)),
      chat_id_(std::move(chat_id)),
      proxy_(std::move(proxy)) {}

void TelegramBot::SetCredentials(std::string token, std::string chat_id,
                                 std::string proxy) {
  token_ = std::move(token);
  chat_id_ = std::move(chat_id);
  proxy_ = std::move(proxy);
}

void TelegramBot::SetCurlTimeout(long timeout_sec) {
  curl_timeout_sec_ = timeout_sec;
}

bool TelegramBot::IsConfigured() const {
  return !token_.empty() && !chat_id_.empty();
}

bool TelegramBot::SendAlert(const std::string& alert) {
  if (!IsConfigured()) return false;

  // 1. Умный указатель для CURL*
  std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> curl(curl_easy_init(),
                                                           curl_easy_cleanup);
  if (!curl) return false;

  std::string message = std::format("🚨 <b> {} </b> 🚨\n", alert);

  // 2. Умный указатель для экранированной строки
  std::unique_ptr<char, decltype(&curl_free)> escaped_message(
      curl_easy_escape(curl.get(), message.c_str(),
                       static_cast<int>(message.length())),
      curl_free);
  if (!escaped_message) return false;

  std::string post_data = std::format("chat_id={}&text={}&parse_mode=HTML",
                                      chat_id_, escaped_message.get());

  std::string url =
      std::format("https://api.telegram.org/bot{}/sendMessage", token_);

  // 3. Умный указатель для глушения ответа в stdout
  std::unique_ptr<FILE, int (*)(FILE*)> dev_null(
      fopen("/dev/null", "w"), static_cast<int (*)(FILE*)>(&fclose));

  // Настройка параметров curl (передаем сырые указатели через .get())
  curl_easy_setopt(curl.get(), CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDS, post_data.c_str());
  curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT, curl_timeout_sec_);
  if (!proxy_.empty()) {
    curl_easy_setopt(curl.get(), CURLOPT_PROXY, proxy_.c_str());
  }
  if (dev_null) {
    curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, dev_null.get());
  }

  CURLcode res = curl_easy_perform(curl.get());
  return res == CURLE_OK;
}

}  // namespace s21
