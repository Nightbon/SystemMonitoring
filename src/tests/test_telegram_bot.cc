#include <gtest/gtest.h>

#include "model/telegram_bot.h"

using namespace s21;

// ======================== Конструктор по умолчанию ========================

TEST(TelegramBotTest, DefaultConstructionNotConfigured) {
  TelegramBot bot;
  EXPECT_FALSE(bot.IsConfigured());
}

// ======================== Конструктор с параметрами ========================

TEST(TelegramBotTest, ParameterizedConstructionIsConfigured) {
  TelegramBot bot("token123", "chat456");
  EXPECT_TRUE(bot.IsConfigured());
}

// ======================== SetCredentials ========================

TEST(TelegramBotTest, SetCredentialsFromDefault) {
  TelegramBot bot;
  EXPECT_FALSE(bot.IsConfigured());

  bot.SetCredentials("my_token", "my_chat");
  EXPECT_TRUE(bot.IsConfigured());
}

TEST(TelegramBotTest, SetCredentialsOverride) {
  TelegramBot bot("old_token", "old_chat");
  EXPECT_TRUE(bot.IsConfigured());

  bot.SetCredentials("new_token", "new_chat");
  EXPECT_TRUE(bot.IsConfigured());
}

// ======================== IsConfigured ========================

TEST(TelegramBotTest, NotConfiguredWhenTokenEmpty) {
  TelegramBot bot("", "chat_id");
  EXPECT_FALSE(bot.IsConfigured());
}

TEST(TelegramBotTest, NotConfiguredWhenChatIdEmpty) {
  TelegramBot bot("token", "");
  EXPECT_FALSE(bot.IsConfigured());
}

TEST(TelegramBotTest, NotConfiguredWhenBothEmpty) {
  TelegramBot bot("", "");
  EXPECT_FALSE(bot.IsConfigured());
}

TEST(TelegramBotTest, ConfiguredWhenBothPresent) {
  TelegramBot bot("tok", "cid");
  EXPECT_TRUE(bot.IsConfigured());
}

// ======================== SendAlert ========================

TEST(TelegramBotTest, SendAlertReturnsFalseWhenNotConfigured) {
  TelegramBot bot;
  EXPECT_FALSE(bot.SendAlert("test alert"));
}

TEST(TelegramBotTest, SendAlertReturnsFalseWhenOnlyTokenSet) {
  TelegramBot bot("token", "");
  EXPECT_FALSE(bot.SendAlert("test alert"));
}