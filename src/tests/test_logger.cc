#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

#include "model/logger.h"

using namespace s21;

class LoggerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    log_dir_ = std::filesystem::temp_directory_path() / "s21_test_logs";
    std::filesystem::remove_all(log_dir_);
  }

  void TearDown() override { std::filesystem::remove_all(log_dir_); }

  std::filesystem::path log_dir_;
};

TEST_F(LoggerTest, CreatesLogDirectory) {
  Logger logger(log_dir_);
  EXPECT_TRUE(std::filesystem::exists(log_dir_));
  EXPECT_TRUE(std::filesystem::is_directory(log_dir_));
}

TEST_F(LoggerTest, LogSingleMetric) {
  Logger logger(log_dir_);
  std::vector<Metric> metrics = {{"cpu", 75.5}};
  logger.LogMetrics(metrics);

  auto lines = logger.GetLastLines(10);
  ASSERT_FALSE(lines.empty());
  EXPECT_NE(lines.back().find("cpu"), std::string::npos);
  EXPECT_NE(lines.back().find("75.50"), std::string::npos);
}

TEST_F(LoggerTest, LogMultipleMetrics) {
  Logger logger(log_dir_);
  std::vector<Metric> metrics = {
      {"cpu", 50.0},
      {"ram", 80.0},
      {"disk", 95.5},
  };
  logger.LogMetrics(metrics);

  auto lines = logger.GetLastLines(10);
  ASSERT_FALSE(lines.empty());
  std::string last = lines.back();
  EXPECT_NE(last.find("cpu"), std::string::npos);
  EXPECT_NE(last.find("50.00"), std::string::npos);
  EXPECT_NE(last.find("ram"), std::string::npos);
  EXPECT_NE(last.find("80.00"), std::string::npos);
  EXPECT_NE(last.find("disk"), std::string::npos);
  EXPECT_NE(last.find("95.50"), std::string::npos);
}

TEST_F(LoggerTest, LogIntMetric) {
  Logger logger(log_dir_);
  std::vector<Metric> metrics = {{"processes", 42}};
  logger.LogMetrics(metrics);

  auto lines = logger.GetLastLines(10);
  ASSERT_FALSE(lines.empty());
  EXPECT_NE(lines.back().find("42"), std::string::npos);
}

TEST_F(LoggerTest, LogStringMetric) {
  Logger logger(log_dir_);
  std::vector<Metric> metrics = {{"status", std::string("OK")}};
  logger.LogMetrics(metrics);

  auto lines = logger.GetLastLines(10);
  ASSERT_FALSE(lines.empty());
  EXPECT_NE(lines.back().find("OK"), std::string::npos);
}

TEST_F(LoggerTest, LogMultipleEntries) {
  Logger logger(log_dir_);

  std::vector<Metric> m1 = {{"cpu", 10.0}};
  std::vector<Metric> m2 = {{"cpu", 20.0}};
  std::vector<Metric> m3 = {{"cpu", 30.0}};
  logger.LogMetrics(m1);
  logger.LogMetrics(m2);
  logger.LogMetrics(m3);

  auto lines = logger.GetLastLines(10);
  EXPECT_EQ(lines.size(), 3u);
}

TEST_F(LoggerTest, GetLastLinesLimit) {
  Logger logger(log_dir_);

  for (int i = 0; i < 10; ++i) {
    std::vector<Metric> m = {{"value", static_cast<double>(i)}};
    logger.LogMetrics(m);
  }

  auto lines = logger.GetLastLines(3);
  EXPECT_EQ(lines.size(), 3u);
}

TEST_F(LoggerTest, GetLastLinesMoreThanAvailable) {
  Logger logger(log_dir_);

  std::vector<Metric> m1 = {{"cpu", 1.0}};
  std::vector<Metric> m2 = {{"cpu", 2.0}};
  logger.LogMetrics(m1);
  logger.LogMetrics(m2);

  auto lines = logger.GetLastLines(100);
  EXPECT_EQ(lines.size(), 2u);
}

TEST_F(LoggerTest, GetLastLinesEmptyLog) {
  Logger logger(log_dir_);
  auto lines = logger.GetLastLines(10);
  EXPECT_TRUE(lines.empty());
}

TEST_F(LoggerTest, GetLastLinesZeroCount) {
  Logger logger(log_dir_);
  std::vector<Metric> m = {{"cpu", 1.0}};
  logger.LogMetrics(m);
  auto lines = logger.GetLastLines(0);
  EXPECT_TRUE(lines.empty());
}

TEST_F(LoggerTest, LogLineContainsTimestamp) {
  Logger logger(log_dir_);
  std::vector<Metric> m = {{"cpu", 1.0}};
  logger.LogMetrics(m);

  auto lines = logger.GetLastLines(1);
  ASSERT_EQ(lines.size(), 1u);
  // Формат: [yy-mm-dd HH:MM:SS] | ...
  EXPECT_NE(lines[0].find("["), std::string::npos);
  EXPECT_NE(lines[0].find("]"), std::string::npos);
}

TEST_F(LoggerTest, LogLineFormat) {
  Logger logger(log_dir_);
  std::vector<Metric> m = {{"cpu", 50.0}, {"mem", 80.0}};
  logger.LogMetrics(m);

  auto lines = logger.GetLastLines(1);
  ASSERT_EQ(lines.size(), 1u);
  // Разделитель " | "
  EXPECT_NE(lines[0].find(" | "), std::string::npos);
}

TEST_F(LoggerTest, ReplacesFileIfItIsRegularFile) {
  std::ofstream dummy(log_dir_);
  dummy << "not a directory";
  dummy.close();
  ASSERT_TRUE(std::filesystem::is_regular_file(log_dir_));

  Logger logger(log_dir_);
  EXPECT_TRUE(std::filesystem::is_directory(log_dir_));
}

TEST_F(LoggerTest, SurvivesAfterDestruction) {
  {
    Logger logger(log_dir_);
    std::vector<Metric> m = {{"cpu", 42.0}};
    logger.LogMetrics(m);
  }

  // New logger should read existing entries
  Logger logger2(log_dir_);
  auto lines = logger2.GetLastLines(10);
  EXPECT_GE(lines.size(), 1u);
}

TEST_F(LoggerTest, LogEmptyMetricsSpan) {
  Logger logger(log_dir_);
  std::vector<Metric> empty;
  logger.LogMetrics(empty);
  // Logger всё равно пишет строку с таймстемпом (пустые метрики не фильтруются)
  auto lines = logger.GetLastLines(10);
  ASSERT_EQ(lines.size(), 1u);
  EXPECT_NE(lines[0].find("["), std::string::npos);
}

TEST_F(LoggerTest, NegativeValueMetric) {
  Logger logger(log_dir_);
  std::vector<Metric> m = {{"temp", -10.5}};
  logger.LogMetrics(m);

  auto lines = logger.GetLastLines(1);
  ASSERT_EQ(lines.size(), 1u);
  EXPECT_NE(lines[0].find("-10.50"), std::string::npos);
}

TEST_F(LoggerTest, ZeroValueMetric) {
  Logger logger(log_dir_);
  std::vector<Metric> m = {{"count", 0.0}};
  logger.LogMetrics(m);

  auto lines = logger.GetLastLines(1);
  ASSERT_EQ(lines.size(), 1u);
  EXPECT_NE(lines[0].find("0.00"), std::string::npos);
}