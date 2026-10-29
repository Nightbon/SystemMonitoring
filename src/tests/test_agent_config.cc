#include <gtest/gtest.h>

#include "model/agent_config.h"

using namespace s21;

// ======================== AgentTypeToString ========================

TEST(AgentTypeToStringTest, CpuReturnsCPU) {
  EXPECT_STREQ(AgentTypeToString(AgentType::kCpu), "CPU");
}

TEST(AgentTypeToStringTest, MemoryReturnsMemory) {
  EXPECT_STREQ(AgentTypeToString(AgentType::kMemory), "Memory");
}

TEST(AgentTypeToStringTest, NetworkReturnsNetwork) {
  EXPECT_STREQ(AgentTypeToString(AgentType::kNetwork), "Network");
}

TEST(AgentTypeToStringTest, UnknownReturnsUnknown) {
  EXPECT_STREQ(AgentTypeToString(AgentType::kUnknown), "Unknown");
}

// ======================== CriticalThreshold ========================

TEST(CriticalThresholdTest, DefaultConstruction) {
  CriticalThreshold th;
  EXPECT_TRUE(th.metric_name.empty());
  EXPECT_TRUE(th.op.empty());
  EXPECT_DOUBLE_EQ(th.value, 0.0);
}

TEST(CriticalThresholdTest, CustomConstruction) {
  CriticalThreshold th{"cpu", ">=", 90.0};
  EXPECT_EQ(th.metric_name, "cpu");
  EXPECT_EQ(th.op, ">=");
  EXPECT_DOUBLE_EQ(th.value, 90.0);
}

TEST(CriticalThresholdTest, Equality) {
  CriticalThreshold a{"cpu", ">=", 90.0};
  CriticalThreshold b{"cpu", ">=", 90.0};
  CriticalThreshold c{"mem", ">=", 90.0};
  EXPECT_EQ(a, b);
  EXPECT_NE(a, c);
}

TEST(CriticalThresholdTest, Ordering) {
  CriticalThreshold a{"a_metric", ">", 1.0};
  CriticalThreshold b{"z_metric", "<", 2.0};
  EXPECT_LT(a, b);
}

// ======================== AgentConfig ========================

TEST(AgentConfigTest, DefaultConstruction) {
  AgentConfig cfg;
  EXPECT_TRUE(cfg.name.empty());
  EXPECT_EQ(cfg.type, AgentType::kUnknown);
  EXPECT_TRUE(cfg.metrics.empty());
  EXPECT_TRUE(cfg.thresholds.empty());
  EXPECT_EQ(cfg.update_interval_seconds, 5);
  EXPECT_TRUE(cfg.config_path.empty());
}

TEST(AgentConfigTest, CustomConstruction) {
  AgentConfig cfg;
  cfg.name = "test_agent";
  cfg.type = AgentType::kCpu;
  cfg.metrics = {"cpu", "processes"};
  cfg.thresholds = {{"cpu", ">=", 90.0}};
  cfg.update_interval_seconds = 10;
  cfg.config_path = "/tmp/test.conf";

  EXPECT_EQ(cfg.name, "test_agent");
  EXPECT_EQ(cfg.type, AgentType::kCpu);
  EXPECT_EQ(cfg.metrics.size(), 2u);
  EXPECT_EQ(cfg.metrics[0], "cpu");
  EXPECT_EQ(cfg.metrics[1], "processes");
  EXPECT_EQ(cfg.thresholds.size(), 1u);
  EXPECT_EQ(cfg.thresholds[0].metric_name, "cpu");
  EXPECT_EQ(cfg.update_interval_seconds, 10);
  EXPECT_EQ(cfg.config_path, "/tmp/test.conf");
}

TEST(AgentConfigTest, Equality) {
  AgentConfig a;
  a.name = "agent";
  a.type = AgentType::kMemory;

  AgentConfig b;
  b.name = "agent";
  b.type = AgentType::kMemory;

  AgentConfig c;
  c.name = "other";

  EXPECT_EQ(a, b);
  EXPECT_NE(a, c);
}

TEST(AgentConfigTest, OrderingByName) {
  AgentConfig a;
  a.name = "alpha";

  AgentConfig b;
  b.name = "beta";

  EXPECT_LT(a, b);
}

TEST(AgentConfigTest, MultipleThresholds) {
  AgentConfig cfg;
  cfg.thresholds = {
      {"cpu", ">=", 90.0},
      {"ram", ">=", 80.0},
      {"disk", ">", 95.0},
  };
  ASSERT_EQ(cfg.thresholds.size(), 3u);
  EXPECT_EQ(cfg.thresholds[1].metric_name, "ram");
  EXPECT_DOUBLE_EQ(cfg.thresholds[2].value, 95.0);
}