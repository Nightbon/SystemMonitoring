#include <gtest/gtest.h>

#include <cstdio>
#include <filesystem>
#include <fstream>

#include "model/config_parser.h"

using namespace s21;

class ConfigParserTest : public ::testing::Test {
 protected:
  void SetUp() override {
    test_dir_ = std::filesystem::temp_directory_path() / "s21_test_configs";
    std::filesystem::create_directories(test_dir_);
  }

  void TearDown() override { std::filesystem::remove_all(test_dir_); }

  std::filesystem::path MakeConfig(const std::string& content) {
    auto path = test_dir_ / "test.conf";
    std::ofstream f(path);
    f << content;
    f.close();
    return path.string();
  }

  std::filesystem::path test_dir_;
};

// ======================== Parse — базовые случаи ========================

TEST_F(ConfigParserTest, ParseMinimalConfig) {
  auto path = MakeConfig(
      "name=agent\n"
      "type=cpu\n");
  auto cfg = ConfigParser::Parse(path.string());
  EXPECT_EQ(cfg.name, "agent");
  EXPECT_EQ(cfg.type, AgentType::kCpu);
  EXPECT_TRUE(cfg.metrics.empty());
  EXPECT_TRUE(cfg.thresholds.empty());
  EXPECT_EQ(cfg.update_interval_seconds, 5);  // default
}

TEST_F(ConfigParserTest, ParseFullConfig) {
  auto path = MakeConfig(
      "name=cpu_agent\n"
      "type=cpu\n"
      "metrics=cpu,processes\n"
      "update_interval=60\n"
      "critical_cpu=>=90\n"
      "critical_processes=>500\n");
  auto cfg = ConfigParser::Parse(path.string());
  EXPECT_EQ(cfg.name, "cpu_agent");
  EXPECT_EQ(cfg.type, AgentType::kCpu);
  ASSERT_EQ(cfg.metrics.size(), 2u);
  EXPECT_EQ(cfg.metrics[0], "cpu");
  EXPECT_EQ(cfg.metrics[1], "processes");
  EXPECT_EQ(cfg.update_interval_seconds, 60);
  ASSERT_EQ(cfg.thresholds.size(), 2u);
  EXPECT_EQ(cfg.thresholds[0].metric_name, "cpu");
  EXPECT_EQ(cfg.thresholds[0].op, ">=");
  EXPECT_DOUBLE_EQ(cfg.thresholds[0].value, 90.0);
  EXPECT_EQ(cfg.thresholds[1].metric_name, "processes");
  EXPECT_EQ(cfg.thresholds[1].op, ">");
  EXPECT_DOUBLE_EQ(cfg.thresholds[1].value, 500.0);
}

// ======================== Parse — типы агентов ========================

TEST_F(ConfigParserTest, ParseTypeMemory) {
  auto path = MakeConfig(
      "name=m\n"
      "type=memory\n");
  auto cfg = ConfigParser::Parse(path.string());
  EXPECT_EQ(cfg.type, AgentType::kMemory);
}

TEST_F(ConfigParserTest, ParseTypeNetwork) {
  auto path = MakeConfig(
      "name=n\n"
      "type=network\n");
  auto cfg = ConfigParser::Parse(path.string());
  EXPECT_EQ(cfg.type, AgentType::kNetwork);
}

TEST_F(ConfigParserTest, ParseTypeUnknown) {
  auto path = MakeConfig(
      "name=x\n"
      "type=invalid\n");
  auto cfg = ConfigParser::Parse(path.string());
  EXPECT_EQ(cfg.type, AgentType::kUnknown);
}

TEST_F(ConfigParserTest, ParseMissingTypeDefaultsToUnknown) {
  auto path = MakeConfig("name=notype\n");
  auto cfg = ConfigParser::Parse(path.string());
  EXPECT_EQ(cfg.type, AgentType::kUnknown);
}

// ======================== Parse — метрики ========================

TEST_F(ConfigParserTest, ParseSingleMetric) {
  auto path = MakeConfig(
      "name=a\n"
      "metrics=cpu\n");
  auto cfg = ConfigParser::Parse(path.string());
  ASSERT_EQ(cfg.metrics.size(), 1u);
  EXPECT_EQ(cfg.metrics[0], "cpu");
}

TEST_F(ConfigParserTest, ParseMultipleMetrics) {
  auto path = MakeConfig(
      "name=a\n"
      "metrics=cpu,mem,disk,net\n");
  auto cfg = ConfigParser::Parse(path.string());
  ASSERT_EQ(cfg.metrics.size(), 4u);
  EXPECT_EQ(cfg.metrics[0], "cpu");
  EXPECT_EQ(cfg.metrics[1], "mem");
  EXPECT_EQ(cfg.metrics[2], "disk");
  EXPECT_EQ(cfg.metrics[3], "net");
}

TEST_F(ConfigParserTest, ParseMetricsWithSpaces) {
  auto path = MakeConfig(
      "name=a\n"
      "metrics= cpu , mem , disk \n");
  auto cfg = ConfigParser::Parse(path.string());
  ASSERT_EQ(cfg.metrics.size(), 3u);
  EXPECT_EQ(cfg.metrics[0], "cpu");
  EXPECT_EQ(cfg.metrics[1], "mem");
  EXPECT_EQ(cfg.metrics[2], "disk");
}

TEST_F(ConfigParserTest, ParseMetricsWithDots) {
  auto path = MakeConfig(
      "name=a\n"
      "metrics=google.com,inet_throughput\n");
  auto cfg = ConfigParser::Parse(path.string());
  ASSERT_EQ(cfg.metrics.size(), 2u);
  EXPECT_EQ(cfg.metrics[0], "google.com");
  EXPECT_EQ(cfg.metrics[1], "inet_throughput");
}

// ======================== Parse — update_interval ========================

TEST_F(ConfigParserTest, ParseUpdateInterval) {
  auto path = MakeConfig(
      "name=a\n"
      "update_interval=120\n");
  auto cfg = ConfigParser::Parse(path.string());
  EXPECT_EQ(cfg.update_interval_seconds, 120);
}

TEST_F(ConfigParserTest, ParseUpdateIntervalDefault) {
  auto path = MakeConfig("name=a\n");
  auto cfg = ConfigParser::Parse(path.string());
  EXPECT_EQ(cfg.update_interval_seconds, 5);
}

TEST_F(ConfigParserTest, ParseUpdateIntervalInvalidUsesDefault) {
  auto path = MakeConfig(
      "name=a\n"
      "update_interval=abc\n");
  auto cfg = ConfigParser::Parse(path.string());
  EXPECT_EQ(cfg.update_interval_seconds, 5);
}

TEST_F(ConfigParserTest, ParseUpdateIntervalZero) {
  auto path = MakeConfig(
      "name=a\n"
      "update_interval=0\n");
  auto cfg = ConfigParser::Parse(path.string());
  EXPECT_EQ(cfg.update_interval_seconds, 0);
}

// ======================== Parse — пороговые значения ========================

TEST_F(ConfigParserTest, ParseCriticalLessThan) {
  auto path = MakeConfig(
      "name=a\n"
      "critical_google.com=<1\n");
  auto cfg = ConfigParser::Parse(path.string());
  ASSERT_EQ(cfg.thresholds.size(), 1u);
  EXPECT_EQ(cfg.thresholds[0].metric_name, "google.com");
  EXPECT_EQ(cfg.thresholds[0].op, "<");
  EXPECT_DOUBLE_EQ(cfg.thresholds[0].value, 1.0);
}

TEST_F(ConfigParserTest, ParseCriticalLessOrEqual) {
  auto path = MakeConfig(
      "name=a\n"
      "critical_cpu=<=50\n");
  auto cfg = ConfigParser::Parse(path.string());
  ASSERT_EQ(cfg.thresholds.size(), 1u);
  EXPECT_EQ(cfg.thresholds[0].op, "<=");
  EXPECT_DOUBLE_EQ(cfg.thresholds[0].value, 50.0);
}

TEST_F(ConfigParserTest, ParseCriticalEqual) {
  // Парсер разделяет по первому '=', поэтому value = "=100",
  // op_len=1 (value[1]='1' != '='), в итоге op="=", val=100
  auto path = MakeConfig(
      "name=a\n"
      "critical_val==100\n");
  auto cfg = ConfigParser::Parse(path.string());
  ASSERT_EQ(cfg.thresholds.size(), 1u);
  EXPECT_EQ(cfg.thresholds[0].op, "=");
  EXPECT_DOUBLE_EQ(cfg.thresholds[0].value, 100.0);
}

TEST_F(ConfigParserTest, ParseCriticalGreaterOrEqual) {
  auto path = MakeConfig(
      "name=a\n"
      "critical_ram=>=80\n");
  auto cfg = ConfigParser::Parse(path.string());
  ASSERT_EQ(cfg.thresholds.size(), 1u);
  EXPECT_EQ(cfg.thresholds[0].op, ">=");
  EXPECT_DOUBLE_EQ(cfg.thresholds[0].value, 80.0);
}

TEST_F(ConfigParserTest, ParseCriticalGreaterThan) {
  auto path = MakeConfig(
      "name=a\n"
      "critical_disk=>95\n");
  auto cfg = ConfigParser::Parse(path.string());
  ASSERT_EQ(cfg.thresholds.size(), 1u);
  EXPECT_EQ(cfg.thresholds[0].op, ">");
  EXPECT_DOUBLE_EQ(cfg.thresholds[0].value, 95.0);
}

TEST_F(ConfigParserTest, ParseMultipleCriticalThresholds) {
  auto path = MakeConfig(
      "name=a\n"
      "critical_cpu=>=90\n"
      "critical_ram=>=80\n"
      "critical_disk=>=95\n");
  auto cfg = ConfigParser::Parse(path.string());
  ASSERT_EQ(cfg.thresholds.size(), 3u);
  EXPECT_EQ(cfg.thresholds[0].metric_name, "cpu");
  EXPECT_EQ(cfg.thresholds[1].metric_name, "ram");
  EXPECT_EQ(cfg.thresholds[2].metric_name, "disk");
}

TEST_F(ConfigParserTest, ParseCriticalFractionalValue) {
  auto path = MakeConfig(
      "name=a\n"
      "critical_temp=>=72.5\n");
  auto cfg = ConfigParser::Parse(path.string());
  ASSERT_EQ(cfg.thresholds.size(), 1u);
  EXPECT_DOUBLE_EQ(cfg.thresholds[0].value, 72.5);
}

TEST_F(ConfigParserTest, ParseCriticalNegativeValue) {
  auto path = MakeConfig(
      "name=a\n"
      "critical_temp=<-10\n");
  auto cfg = ConfigParser::Parse(path.string());
  ASSERT_EQ(cfg.thresholds.size(), 1u);
  EXPECT_DOUBLE_EQ(cfg.thresholds[0].value, -10.0);
}

// ======================== Parse — обработка файлов ========================

TEST_F(ConfigParserTest, ParseNonexistentFileThrows) {
  EXPECT_THROW(
      { (void)ConfigParser::Parse("/nonexistent/path/file.conf"); },
      std::runtime_error);
}

TEST_F(ConfigParserTest, ParseConfigPathStored) {
  auto path = MakeConfig("name=a\n");
  auto cfg = ConfigParser::Parse(path.string());
  EXPECT_EQ(cfg.config_path, path.string());
}

// ======================== Parse — комментарии и пустые строки
// ========================

TEST_F(ConfigParserTest, ParseIgnoresComments) {
  auto path = MakeConfig(
      "# This is a comment\n"
      "name=test\n"
      "# Another comment\n"
      "type=cpu\n");
  auto cfg = ConfigParser::Parse(path.string());
  EXPECT_EQ(cfg.name, "test");
  EXPECT_EQ(cfg.type, AgentType::kCpu);
}

TEST_F(ConfigParserTest, ParseIgnoresEmptyLines) {
  auto path = MakeConfig(
      "\n"
      "\n"
      "name=test\n"
      "\n"
      "type=cpu\n"
      "\n");
  auto cfg = ConfigParser::Parse(path.string());
  EXPECT_EQ(cfg.name, "test");
}

TEST_F(ConfigParserTest, ParseSkipsLinesWithoutEquals) {
  auto path = MakeConfig(
      "name=test\n"
      "invalid_line_without_equals\n"
      "type=cpu\n");
  auto cfg = ConfigParser::Parse(path.string());
  EXPECT_EQ(cfg.name, "test");
  EXPECT_EQ(cfg.type, AgentType::kCpu);
}

TEST_F(ConfigParserTest, ParseTrimsWhitespace) {
  auto path = MakeConfig(
      "  name  =  test  \n"
      "  type  =  memory  \n");
  auto cfg = ConfigParser::Parse(path.string());
  EXPECT_EQ(cfg.name, "test");
  EXPECT_EQ(cfg.type, AgentType::kMemory);
}

TEST_F(ConfigParserTest, ParseHandlesCarriageReturn) {
  auto path = MakeConfig("name=test\r\ntype=cpu\r\n");
  auto cfg = ConfigParser::Parse(path.string());
  EXPECT_EQ(cfg.name, "test");
  EXPECT_EQ(cfg.type, AgentType::kCpu);
}

// ======================== Save ========================

TEST_F(ConfigParserTest, SaveAndReparseRoundTrip) {
  AgentConfig orig;
  orig.name = "my_agent";
  orig.type = AgentType::kMemory;
  orig.metrics = {"ram", "swap"};
  orig.update_interval_seconds = 30;
  orig.thresholds = {{"ram", ">=", 80.0}};
  orig.config_path = (test_dir_ / "saved.conf").string();

  ConfigParser::Save(orig.config_path, orig);

  auto loaded = ConfigParser::Parse(orig.config_path);
  EXPECT_EQ(loaded.name, orig.name);
  EXPECT_EQ(loaded.type, orig.type);
  EXPECT_EQ(loaded.metrics, orig.metrics);
  EXPECT_EQ(loaded.update_interval_seconds, orig.update_interval_seconds);
  ASSERT_EQ(loaded.thresholds.size(), orig.thresholds.size());
  EXPECT_EQ(loaded.thresholds[0].metric_name, orig.thresholds[0].metric_name);
  EXPECT_EQ(loaded.thresholds[0].op, orig.thresholds[0].op);
  EXPECT_DOUBLE_EQ(loaded.thresholds[0].value, orig.thresholds[0].value);
}

TEST_F(ConfigParserTest, SaveFullConfig) {
  AgentConfig cfg;
  cfg.name = "cpu_agent";
  cfg.type = AgentType::kCpu;
  cfg.metrics = {"cpu", "processes"};
  cfg.update_interval_seconds = 60;
  cfg.thresholds = {
      {"cpu", ">=", 90.0},
      {"processes", ">", 500.0},
  };

  auto path = (test_dir_ / "full.conf").string();
  ConfigParser::Save(path, cfg);

  std::ifstream f(path);
  std::string content((std::istreambuf_iterator<char>(f)),
                      std::istreambuf_iterator<char>());

  EXPECT_NE(content.find("name=cpu_agent"), std::string::npos);
  EXPECT_NE(content.find("type=cpu"), std::string::npos);
  EXPECT_NE(content.find("metrics=cpu,processes"), std::string::npos);
  EXPECT_NE(content.find("update_interval=60"), std::string::npos);
  EXPECT_NE(content.find("critical_cpu=>=90"), std::string::npos);
  EXPECT_NE(content.find("critical_processes=>500"), std::string::npos);
}

TEST_F(ConfigParserTest, SaveUnknownType) {
  AgentConfig cfg;
  cfg.name = "unknown_agent";
  cfg.type = AgentType::kUnknown;

  auto path = (test_dir_ / "unknown.conf").string();
  ConfigParser::Save(path, cfg);

  auto loaded = ConfigParser::Parse(path);
  EXPECT_EQ(loaded.type, AgentType::kUnknown);
}

TEST_F(ConfigParserTest, SaveEmptyMetrics) {
  AgentConfig cfg;
  cfg.name = "empty";
  cfg.type = AgentType::kCpu;

  auto path = (test_dir_ / "empty_metrics.conf").string();
  ConfigParser::Save(path, cfg);

  auto loaded = ConfigParser::Parse(path);
  EXPECT_TRUE(loaded.metrics.empty());
}

TEST_F(ConfigParserTest, SaveToInvalidPathThrows) {
  AgentConfig cfg;
  cfg.name = "test";
  EXPECT_THROW(ConfigParser::Save("/nonexistent/dir/impossible/file.conf", cfg),
               std::runtime_error);
}

TEST_F(ConfigParserTest, SaveNetworkType) {
  AgentConfig cfg;
  cfg.name = "net";
  cfg.type = AgentType::kNetwork;
  auto path = (test_dir_ / "net.conf").string();
  ConfigParser::Save(path, cfg);

  auto loaded = ConfigParser::Parse(path);
  EXPECT_EQ(loaded.type, AgentType::kNetwork);
}

// ======================== Parse — реальный конфиг ========================

TEST_F(ConfigParserTest, ParseRealCpuAgentConfig) {
  auto src_path =
      std::filesystem::path(TEST_DATA_DIR) / "src/agents/cpu_agent.conf";
  if (!std::filesystem::exists(src_path))
    GTEST_SKIP() << "cpu_agent.conf not found";
  auto cfg = ConfigParser::Parse(src_path.string());
  EXPECT_EQ(cfg.name, "cpu_agent");
  EXPECT_EQ(cfg.type, AgentType::kCpu);
  EXPECT_TRUE(cfg.update_interval_seconds);
  ASSERT_EQ(cfg.thresholds.size(), 2u);
}

TEST_F(ConfigParserTest, ParseRealMemoryAgentConfig) {
  auto src_path =
      std::filesystem::path(TEST_DATA_DIR) / "src/agents/memory_agent.conf";
  if (!std::filesystem::exists(src_path))
    GTEST_SKIP() << "memory_agent.conf not found";
  auto cfg = ConfigParser::Parse(src_path.string());
  EXPECT_EQ(cfg.name, "memory_agent");
  EXPECT_EQ(cfg.type, AgentType::kMemory);
  ASSERT_EQ(cfg.metrics.size(), 6u);
  EXPECT_EQ(cfg.metrics[0], "ram_total");
  ASSERT_EQ(cfg.thresholds.size(), 3u);
}

TEST_F(ConfigParserTest, ParseRealNetworkAgentConfig) {
  auto src_path =
      std::filesystem::path(TEST_DATA_DIR) / "src/agents/network_agent.conf";
  if (!std::filesystem::exists(src_path))
    GTEST_SKIP() << "network_agent.conf not found";
  auto cfg = ConfigParser::Parse(src_path.string());
  EXPECT_EQ(cfg.name, "network_agent");
  EXPECT_EQ(cfg.type, AgentType::kNetwork);
  ASSERT_EQ(cfg.thresholds.size(), 1u);
  EXPECT_EQ(cfg.thresholds[0].op, "<");
  EXPECT_DOUBLE_EQ(cfg.thresholds[0].value, 1.0);
}