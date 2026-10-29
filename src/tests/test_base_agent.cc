#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <thread>

#include "model/base_agent.h"

using namespace s21;

class BaseAgentTest : public ::testing::Test {
 protected:
  void SetUp() override {
    test_dir_ = std::filesystem::temp_directory_path() / "s21_test_base_agent";
    std::filesystem::remove_all(test_dir_);
    std::filesystem::create_directories(test_dir_);
  }

  void TearDown() override { std::filesystem::remove_all(test_dir_); }

  std::filesystem::path MakeConfig(const std::string& content,
                                   const std::string& filename = "agent.conf") {
    auto path = test_dir_ / filename;
    std::ofstream f(path);
    f << content;
    f.close();
    return path;
  }

  std::filesystem::path test_dir_;
};

// ======================== Конструктор с валидным конфигом
// ========================

class TestableAgent : public BaseAgent {
 public:
  TestableAgent(const std::string& config_path, const std::string& default_name,
                AgentType default_type,
                std::vector<std::string> default_metrics)
      : BaseAgent(config_path, default_name, default_type,
                  std::move(default_metrics)) {}

  std::vector<Metric> UpdateMetrics(std::stop_token stop_token = {}) override {
    return {};
  }
};

TEST_F(BaseAgentTest, ParsesValidConfig) {
  auto path = MakeConfig(
      "name=my_cpu\n"
      "type=cpu\n"
      "metrics=cpu,processes\n"
      "update_interval=30\n");

  TestableAgent agent(path.string(), "fallback", AgentType::kUnknown, {});

  EXPECT_EQ(agent.GetName(), "my_cpu");
  EXPECT_EQ(agent.GetType(), AgentType::kCpu);
  EXPECT_EQ(agent.GetConfig().metrics.size(), 2u);
  EXPECT_EQ(agent.GetConfig().update_interval_seconds, 30);
}

TEST_F(BaseAgentTest, ParsesConfigWithThresholds) {
  auto path = MakeConfig(
      "name=mem_agent\n"
      "type=memory\n"
      "metrics=ram\n"
      "critical_ram=>=80\n");

  TestableAgent agent(path.string(), "fallback", AgentType::kUnknown, {});

  EXPECT_EQ(agent.GetName(), "mem_agent");
  EXPECT_EQ(agent.GetType(), AgentType::kMemory);
  ASSERT_EQ(agent.GetConfig().thresholds.size(), 1u);
  EXPECT_EQ(agent.GetConfig().thresholds[0].metric_name, "ram");
  EXPECT_DOUBLE_EQ(agent.GetConfig().thresholds[0].value, 80.0);
}

// ======================== Fallback при невалидном конфиге
// ========================

TEST_F(BaseAgentTest, FallsBackOnMissingFile) {
  std::string nonexistent = (test_dir_ / "no_such_file.conf").string();
  TestableAgent agent(nonexistent, "default_name", AgentType::kCpu,
                      {"metric1", "metric2"});

  EXPECT_EQ(agent.GetName(), "default_name");
  EXPECT_EQ(agent.GetType(), AgentType::kCpu);
  ASSERT_EQ(agent.GetConfig().metrics.size(), 2u);
  EXPECT_EQ(agent.GetConfig().metrics[0], "metric1");
  EXPECT_EQ(agent.GetConfig().metrics[1], "metric2");
  EXPECT_EQ(agent.GetConfig().update_interval_seconds, ScanAgentsInterval);
}

TEST_F(BaseAgentTest, FallbackSetsConfigPath) {
  std::string path = (test_dir_ / "missing.conf").string();
  TestableAgent agent(path, "def", AgentType::kUnknown, {});

  EXPECT_EQ(agent.GetConfig().config_path, path);
}

TEST_F(BaseAgentTest, FallbackTypeNetwork) {
  std::string path = (test_dir_ / "missing.conf").string();
  TestableAgent agent(path, "net_def", AgentType::kNetwork, {"net"});

  EXPECT_EQ(agent.GetName(), "net_def");
  EXPECT_EQ(agent.GetType(), AgentType::kNetwork);
}

TEST_F(BaseAgentTest, FallbackEmptyMetrics) {
  std::string path = (test_dir_ / "missing.conf").string();
  TestableAgent agent(path, "empty", AgentType::kUnknown, {});

  EXPECT_TRUE(agent.GetConfig().metrics.empty());
}

// ======================== GetConfig / SetConfig ========================

TEST_F(BaseAgentTest, GetConfigReturnsReference) {
  auto path = MakeConfig("name=a\ntype=cpu\n");
  TestableAgent agent(path.string(), "f", AgentType::kUnknown, {});

  const auto& cfg = agent.GetConfig();
  EXPECT_EQ(cfg.name, "a");
  // Изменение через ссылку должно быть видно
  // (проверяем что это действительно ссылка на внутренний объект)
  EXPECT_EQ(&cfg, &agent.GetConfig());
}

TEST_F(BaseAgentTest, SetConfigReplacesConfig) {
  auto path = MakeConfig("name=old\ntype=cpu\n");
  TestableAgent agent(path.string(), "f", AgentType::kUnknown, {});

  AgentConfig new_cfg;
  new_cfg.name = "new_name";
  new_cfg.type = AgentType::kMemory;
  new_cfg.update_interval_seconds = 42;

  agent.SetConfig(new_cfg);

  EXPECT_EQ(agent.GetName(), "new_name");
  EXPECT_EQ(agent.GetType(), AgentType::kMemory);
  EXPECT_EQ(agent.GetConfig().update_interval_seconds, 42);
}

// ======================== GetName / GetType ========================

TEST_F(BaseAgentTest, GetNameFromConfig) {
  auto path = MakeConfig("name=from_config\ntype=memory\n");
  TestableAgent agent(path.string(), "f", AgentType::kUnknown, {});
  EXPECT_EQ(agent.GetName(), "from_config");
}

TEST_F(BaseAgentTest, GetTypeFromConfig) {
  auto path = MakeConfig("name=a\ntype=network\n");
  TestableAgent agent(path.string(), "f", AgentType::kUnknown, {});
  EXPECT_EQ(agent.GetType(), AgentType::kNetwork);
}

// ======================== GetStartTime ========================

TEST_F(BaseAgentTest, StartTimeIsRecent) {
  auto path = MakeConfig("name=a\ntype=cpu\n");
  auto before = std::time(nullptr);
  TestableAgent agent(path.string(), "f", AgentType::kUnknown, {});
  auto after = std::time(nullptr);

  auto start = agent.GetStartTime();
  EXPECT_GE(start, before);
  EXPECT_LE(start, after);
}

// ======================== SetActive / IsActive ========================

TEST_F(BaseAgentTest, ActiveByDefault) {
  auto path = MakeConfig("name=a\ntype=cpu\n");
  TestableAgent agent(path.string(), "f", AgentType::kUnknown, {});
  EXPECT_TRUE(agent.IsActive());
}

TEST_F(BaseAgentTest, Deactivate) {
  auto path = MakeConfig("name=a\ntype=cpu\n");
  TestableAgent agent(path.string(), "f", AgentType::kUnknown, {});
  agent.SetActive(false);
  EXPECT_FALSE(agent.IsActive());
}

TEST_F(BaseAgentTest, Reactivate) {
  auto path = MakeConfig("name=a\ntype=cpu\n");
  TestableAgent agent(path.string(), "f", AgentType::kUnknown, {});
  agent.SetActive(false);
  agent.SetActive(true);
  EXPECT_TRUE(agent.IsActive());
}

TEST_F(BaseAgentTest, SetActiveMultipleTimes) {
  auto path = MakeConfig("name=a\ntype=cpu\n");
  TestableAgent agent(path.string(), "f", AgentType::kUnknown, {});

  for (int i = 0; i < 10; ++i) {
    agent.SetActive(i % 2 == 0);
    EXPECT_EQ(agent.IsActive(), i % 2 == 0);
  }
}

// ======================== UpdateMetrics ========================

TEST_F(BaseAgentTest, UpdateMetricsReturnsEmpty) {
  auto path = MakeConfig("name=a\ntype=cpu\n");
  TestableAgent agent(path.string(), "f", AgentType::kUnknown, {});
  auto metrics = agent.UpdateMetrics();
  EXPECT_TRUE(metrics.empty());
}