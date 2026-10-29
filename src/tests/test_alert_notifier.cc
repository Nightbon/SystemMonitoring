#include <gtest/gtest.h>

#include "model/alert_notifier.h"

using namespace s21;

// ======================== SetTelegramCredentials ========================

TEST(AlertNotifierTest, SetTelegramCredentialsDoesNotCrash) {
  AlertNotifier notifier;
  EXPECT_NO_THROW(notifier.SetTelegramCredentials("token", "chat_id", ""));
}

TEST(AlertNotifierTest, SetTelegramCredentialsEmptyStrings) {
  AlertNotifier notifier;
  EXPECT_NO_THROW(notifier.SetTelegramCredentials("", "", ""));
}

// ======================== SetEmailCredentials ========================

TEST(AlertNotifierTest, SetEmailCredentialsDoesNotCrash) {
  AlertNotifier notifier;
  EXPECT_NO_THROW(notifier.SetEmailCredentials("smtp.example.com", 587, "user",
                                               "pass", "from@example.com"));
}

TEST(AlertNotifierTest, SetEmailCredentialsEmptyStrings) {
  AlertNotifier notifier;
  EXPECT_NO_THROW(notifier.SetEmailCredentials("", 0, "", "", ""));
}

// ======================== SetEmailRecipient ========================

TEST(AlertNotifierTest, SetEmailRecipientDoesNotCrash) {
  AlertNotifier notifier;
  EXPECT_NO_THROW(notifier.SetEmailRecipient("to@example.com"));
}

// ======================== SetEmailEnabled ========================

TEST(AlertNotifierTest, SetEmailEnabledDoesNotCrash) {
  AlertNotifier notifier;
  EXPECT_NO_THROW(notifier.SetEmailEnabled(true));
  EXPECT_NO_THROW(notifier.SetEmailEnabled(false));
}

// ======================== ProcessAlerts ========================

TEST(AlertNotifierTest, ProcessEmptyAlertsDoesNotCrash) {
  AlertNotifier notifier;
  std::vector<std::string> alerts;
  EXPECT_NO_THROW(notifier.ProcessAlerts(alerts));
}

TEST(AlertNotifierTest, ProcessSingleAlertDoesNotCrash) {
  AlertNotifier notifier;
  // Без configured telegram/email — SendAlert просто вернёт false
  std::vector<std::string> alerts = {
      "CRITICAL: cpu = 95.00 (threshold >= 90.00)"};
  EXPECT_NO_THROW(notifier.ProcessAlerts(alerts));
}

TEST(AlertNotifierTest, ProcessMultipleAlertsDoesNotCrash) {
  AlertNotifier notifier;
  std::vector<std::string> alerts = {
      "CRITICAL: cpu = 95.00 (threshold >= 90.00)",
      "CRITICAL: ram = 85.00 (threshold >= 80.00)",
      "CRITICAL: disk = 98.00 (threshold >= 95.00)",
  };
  EXPECT_NO_THROW(notifier.ProcessAlerts(alerts));
}

TEST(AlertNotifierTest, ProcessAlertsWithTelegramConfigured) {
  AlertNotifier notifier;
  notifier.SetTelegramCredentials("fake_token", "fake_chat_id", "");
  notifier.SetTelegramCurlTimeout(1);
  // Реальная отправка не пройдёт, но не должна упасть
  std::vector<std::string> alerts = {"test alert"};
  EXPECT_NO_THROW(notifier.ProcessAlerts(alerts));
}

TEST(AlertNotifierTest, ProcessAlertsWithEmailConfigured) {
  AlertNotifier notifier;
  notifier.SetEmailCredentials("smtp.example.com", 587, "user", "pass",
                               "from@example.com");
  notifier.SetEmailRecipient("to@example.com");
  notifier.SetEmailEnabled(true);
  // Реальная отправка не пройдёт, но не должна упасть
  std::vector<std::string> alerts = {"test alert"};
  EXPECT_NO_THROW(notifier.ProcessAlerts(alerts));
}