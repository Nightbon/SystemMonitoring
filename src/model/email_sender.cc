#include "email_sender.h"

#include <curl/curl.h>

#include <cstdio>
#include <cstring>
#include <format>
#include <memory>
#include <string>

namespace s21 {

namespace {

std::string BuildEmailPayload(const std::string& from, const std::string& to,
                              const std::string& subject,
                              const std::string& body) {
  return std::format(
      "From: {}\r\n"
      "To: {}\r\n"
      "Subject: {}\r\n"
      "\r\n"
      "{}",
      from, to, subject, body);
}

struct FileDeleter {
  void operator()(FILE* fp) const {
    if (fp) std::fclose(fp);
  }
};

}  // namespace

EmailSender::EmailSender(std::string smtp_server, int smtp_port,
                         std::string username, std::string password,
                         std::string from_address)
    : smtp_server_(std::move(smtp_server)),
      smtp_port_(smtp_port),
      username_(std::move(username)),
      password_(std::move(password)),
      from_address_(std::move(from_address)) {}

void EmailSender::SetCredentials(std::string smtp_server, int smtp_port,
                                 std::string username, std::string password,
                                 std::string from_address) {
  smtp_server_ = std::move(smtp_server);
  smtp_port_ = smtp_port;
  username_ = std::move(username);
  password_ = std::move(password);
  from_address_ = std::move(from_address);
}

void EmailSender::SetRecipient(std::string to_address) {
  to_address_ = std::move(to_address);
}

void EmailSender::SetEnabled(bool enabled) { enabled_ = enabled; }

void EmailSender::SetCurlTimeout(long timeout_sec) {
  curl_timeout_sec_ = timeout_sec;
}

bool EmailSender::IsConfigured() const {
  return !smtp_server_.empty() && !username_.empty() && !password_.empty() &&
         !from_address_.empty() && !to_address_.empty();
}

bool EmailSender::IsEnabled() const { return enabled_ && IsConfigured(); }

bool EmailSender::SendAlert(const std::string& alert) {
  if (!IsEnabled()) return false;

  std::string payload =
      BuildEmailPayload(from_address_, to_address_, "CRITICAL Alert", alert);

  // 1. Умный указатель для CURL*
  std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> curl(curl_easy_init(),
                                                           curl_easy_cleanup);
  if (!curl) return false;

  // 2. Умный указатель для виртуального файла в памяти
  std::unique_ptr<FILE, FileDeleter> mem_stream(
      fmemopen(const_cast<char*>(payload.data()), payload.size(), "r"));
  if (!mem_stream) return false;

  // 3. Умный указатель для списка получателей
  std::unique_ptr<struct curl_slist, decltype(&curl_slist_free_all)> recipients(
      curl_slist_append(nullptr, to_address_.c_str()), curl_slist_free_all);
  if (!recipients) return false;

  // 4. Умный указатель для глушения stdout сервера
  std::unique_ptr<FILE, FileDeleter> dev_null(fopen("/dev/null", "w"));

  std::string url = std::format("smtp://{}:{}", smtp_server_, smtp_port_);
  // Настройка libcurl (передаем сырые указатели через .get())
  curl_easy_setopt(curl.get(), CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl.get(), CURLOPT_USE_SSL, (long)CURLUSESSL_ALL);
  curl_easy_setopt(curl.get(), CURLOPT_USERNAME, username_.c_str());
  curl_easy_setopt(curl.get(), CURLOPT_PASSWORD, password_.c_str());
  curl_easy_setopt(curl.get(), CURLOPT_MAIL_FROM, from_address_.c_str());
  curl_easy_setopt(curl.get(), CURLOPT_MAIL_RCPT, recipients.get());
  curl_easy_setopt(curl.get(), CURLOPT_UPLOAD, 1L);
  curl_easy_setopt(curl.get(), CURLOPT_READDATA, mem_stream.get());
  curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT, curl_timeout_sec_);
  if (dev_null) {
    curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, dev_null.get());
  }

  CURLcode res = curl_easy_perform(curl.get());
  return res == CURLE_OK;
}

}  // namespace s21
