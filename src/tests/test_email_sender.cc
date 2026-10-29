#include <gtest/gtest.h>

#include "model/email_sender.h"

using namespace s21;

// ======================== Конструктор по умолчанию ========================

TEST(EmailSenderTest, DefaultConstruction) {
  EmailSender sender;
  EXPECT_FALSE(sender.IsConfigured());
  EXPECT_FALSE(sender.IsEnabled());
  EXPECT_EQ(sender.IsEnabled(), false);
}

// ======================== Конструктор с параметрами ========================

TEST(EmailSenderTest, ParameterizedConstruction) {
  EmailSender sender("smtp.example.com", 587, "user", "pass",
                     "from@example.com");
  EXPECT_FALSE(sender.IsConfigured());  // to_address_ не задан
  EXPECT_FALSE(sender.IsEnabled());
}

// ======================== SetCredentials ========================

TEST(EmailSenderTest, SetCredentialsMakesPartiallyConfigured) {
  EmailSender sender;
  sender.SetCredentials("smtp.example.com", 587, "user", "pass",
                        "from@example.com");
  // Без recipient не полностью сконфигурирован
  EXPECT_FALSE(sender.IsConfigured());
}

TEST(EmailSenderTest, SetCredentialsOverrides) {
  EmailSender sender("old.smtp", 25, "old", "old", "old@old.com");
  sender.SetRecipient("to@old.com");
  EXPECT_TRUE(sender.IsConfigured());

  sender.SetCredentials("new.smtp", 465, "new", "new", "new@new.com");
  EXPECT_TRUE(sender.IsConfigured());
}

// ======================== SetRecipient ========================

TEST(EmailSenderTest, SetRecipientAloneNotEnough) {
  EmailSender sender;
  sender.SetRecipient("to@example.com");
  EXPECT_FALSE(sender.IsConfigured());
}

TEST(EmailSenderTest, FullConfiguration) {
  EmailSender sender;
  sender.SetCredentials("smtp.example.com", 587, "user", "pass",
                        "from@example.com");
  sender.SetRecipient("to@example.com");
  EXPECT_TRUE(sender.IsConfigured());
}

// ======================== IsConfigured ========================

TEST(EmailSenderTest, NotConfiguredDefault) {
  EmailSender sender;
  EXPECT_FALSE(sender.IsConfigured());
}

TEST(EmailSenderTest, NotConfiguredMissingSmtp) {
  EmailSender sender;
  sender.SetCredentials("", 587, "user", "pass", "from@example.com");
  sender.SetRecipient("to@example.com");
  EXPECT_FALSE(sender.IsConfigured());
}

TEST(EmailSenderTest, NotConfiguredMissingUsername) {
  EmailSender sender;
  sender.SetCredentials("smtp.example.com", 587, "", "pass",
                        "from@example.com");
  sender.SetRecipient("to@example.com");
  EXPECT_FALSE(sender.IsConfigured());
}

TEST(EmailSenderTest, NotConfiguredMissingPassword) {
  EmailSender sender;
  sender.SetCredentials("smtp.example.com", 587, "user", "",
                        "from@example.com");
  sender.SetRecipient("to@example.com");
  EXPECT_FALSE(sender.IsConfigured());
}

TEST(EmailSenderTest, NotConfiguredMissingFrom) {
  EmailSender sender;
  sender.SetCredentials("smtp.example.com", 587, "user", "pass", "");
  sender.SetRecipient("to@example.com");
  EXPECT_FALSE(sender.IsConfigured());
}

TEST(EmailSenderTest, NotConfiguredMissingTo) {
  EmailSender sender;
  sender.SetCredentials("smtp.example.com", 587, "user", "pass",
                        "from@example.com");
  EXPECT_FALSE(sender.IsConfigured());
}

// ======================== SetEnabled / IsEnabled ========================

TEST(EmailSenderTest, EnabledRequiresConfiguration) {
  EmailSender sender;
  sender.SetEnabled(true);
  // Не сконфигурирован — не enabled
  EXPECT_FALSE(sender.IsEnabled());
}

TEST(EmailSenderTest, ConfiguredButNotExplicitlyEnabled) {
  EmailSender sender;
  sender.SetCredentials("smtp.example.com", 587, "user", "pass",
                        "from@example.com");
  sender.SetRecipient("to@example.com");
  EXPECT_TRUE(sender.IsConfigured());
  // enabled_ по умолчанию false
  EXPECT_FALSE(sender.IsEnabled());
}

TEST(EmailSenderTest, FullyEnabled) {
  EmailSender sender;
  sender.SetCredentials("smtp.example.com", 587, "user", "pass",
                        "from@example.com");
  sender.SetRecipient("to@example.com");
  sender.SetEnabled(true);
  EXPECT_TRUE(sender.IsEnabled());
}

TEST(EmailSenderTest, DisableAfterEnable) {
  EmailSender sender;
  sender.SetCredentials("smtp.example.com", 587, "user", "pass",
                        "from@example.com");
  sender.SetRecipient("to@example.com");
  sender.SetEnabled(true);
  EXPECT_TRUE(sender.IsEnabled());
  sender.SetEnabled(false);
  EXPECT_FALSE(sender.IsEnabled());
}

// ======================== SendAlert ========================

TEST(EmailSenderTest, SendAlertReturnsFalseWhenNotEnabled) {
  EmailSender sender;
  EXPECT_FALSE(sender.SendAlert("alert"));
}

TEST(EmailSenderTest, SendAlertReturnsFalseWhenConfiguredButNotEnabled) {
  EmailSender sender;
  sender.SetCredentials("smtp.example.com", 587, "user", "pass",
                        "from@example.com");
  sender.SetRecipient("to@example.com");
  EXPECT_FALSE(sender.SendAlert("alert"));
}

TEST(EmailSenderTest, SendAlertReturnsFalseWhenEnabledButNotConfigured) {
  EmailSender sender;
  sender.SetEnabled(true);
  EXPECT_FALSE(sender.SendAlert("alert"));
}

// ======================== Параметры порта ========================

TEST(EmailSenderTest, DifferentPortValues) {
  EmailSender sender;
  sender.SetCredentials("smtp.example.com", 25, "user", "pass",
                        "from@example.com");
  sender.SetRecipient("to@example.com");
  EXPECT_TRUE(sender.IsConfigured());

  sender.SetCredentials("smtp.example.com", 465, "user", "pass",
                        "from@example.com");
  EXPECT_TRUE(sender.IsConfigured());

  sender.SetCredentials("smtp.example.com", 0, "user", "pass",
                        "from@example.com");
  EXPECT_TRUE(sender.IsConfigured());
}