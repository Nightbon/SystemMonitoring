#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <thread>

#include "model/agent_config.h"
#include "model/agent_loader.h"
#include "model/agent_manager.h"
#include "model/i_agent.h"
#include "model/metric.h"

using namespace s21;

class AgentManagerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    agents_dir_ =
        std::filesystem::temp_directory_path() / "s21_test_mgr_agents";
    std::filesystem::remove_all(agents_dir_);
  }

  void TearDown() override { std::filesystem::remove_all(agents_dir_); }

  // Создаёт минимальный .so, который экспортирует символ create_agent
  void CreateFakeSo(const std::string& name) {
    auto so_path = agents_dir_ / (name + ".so");
    // Пишем произвольные байты — файл существует, но не является валидным .so
    std::ofstream f(so_path, std::ios::binary);
    f << "FAKE_SHARED_OBJECT";
    f.close();
  }

  std::filesystem::path agents_dir_;
};

// ======================== Конструкция ========================

TEST_F(AgentManagerTest, ConstructionCreatesDirectory) {
  AgentManager mgr(agents_dir_);
  EXPECT_TRUE(std::filesystem::exists(agents_dir_));
  EXPECT_TRUE(std::filesystem::is_directory(agents_dir_));
}

TEST_F(AgentManagerTest, ConstructionReplacesRegularFile) {
  // Создаём regular file на месте директории
  std::ofstream f(agents_dir_);
  f << "not a dir";
  f.close();
  ASSERT_TRUE(std::filesystem::is_regular_file(agents_dir_));

  AgentManager mgr(agents_dir_);
  EXPECT_TRUE(std::filesystem::is_directory(agents_dir_));
}

TEST_F(AgentManagerTest, DestructionDoesNotCrash) {
  { AgentManager mgr(agents_dir_); }
}

// ======================== GetAgents ========================

TEST_F(AgentManagerTest, GetAgentsEmptyInitially) {
  AgentManager mgr(agents_dir_);
  auto agents = mgr.GetAgents();
  EXPECT_TRUE(agents.empty());
}

TEST_F(AgentManagerTest, GetAgentsReturnsCopy) {
  AgentManager mgr(agents_dir_);
  auto a1 = mgr.GetAgents();
  auto a2 = mgr.GetAgents();
  EXPECT_EQ(a1.size(), a2.size());
}

// ======================== ScanAndLoad ========================

TEST_F(AgentManagerTest, ScanAndLoadEmptyDir) {
  AgentManager mgr(agents_dir_);
  mgr.ScanAndLoad();
  EXPECT_TRUE(mgr.GetAgents().empty());
}

TEST_F(AgentManagerTest, ScanAndLoadWithFakeSoIgnored) {
  // Фейковый .so не содержит символа create_agent — будет проигнорирован
  CreateFakeSo("fake");
  AgentManager mgr(agents_dir_);
  mgr.ScanAndLoad();
  EXPECT_TRUE(mgr.GetAgents().empty());
}

TEST_F(AgentManagerTest, ScanAndLoadNonSoFilesIgnored) {
  // Создаём файлы не .so
  std::ofstream(agents_dir_ / "readme.txt") << "text";
  std::ofstream(agents_dir_ / "script.sh") << "#!/bin/bash";

  AgentManager mgr(agents_dir_);
  mgr.ScanAndLoad();
  EXPECT_TRUE(mgr.GetAgents().empty());
}

TEST_F(AgentManagerTest, ScanAndLoadOnlyFiltersSoExtension) {
  std::ofstream(agents_dir_ / "agent.txt") << "text";
  CreateFakeSo("real_agent");
  std::ofstream(agents_dir_ / "data.json") << "{}";

  AgentManager mgr(agents_dir_);
  mgr.ScanAndLoad();
  // Фейковый .so не загружается (нет символа)
  EXPECT_TRUE(mgr.GetAgents().empty());
}

// ======================== StartScanning / StopScanning
// ========================

TEST_F(AgentManagerTest, StartAndStopDoNotCrash) {
  AgentManager mgr(agents_dir_);
  mgr.StartScanning(1);
  std::this_thread::sleep_for(std::chrono::milliseconds(300));
  mgr.StopScanning();
}

TEST_F(AgentManagerTest, StopWithoutStartDoesNotCrash) {
  AgentManager mgr(agents_dir_);
  mgr.StopScanning();
  mgr.StopScanning();
}

TEST_F(AgentManagerTest, ScanningEmptyDir) {
  AgentManager mgr(agents_dir_);
  mgr.StartScanning(1);
  std::this_thread::sleep_for(std::chrono::milliseconds(300));
  EXPECT_TRUE(mgr.GetAgents().empty());
  mgr.StopScanning();
}

TEST_F(AgentManagerTest, MultipleStartStopCycles) {
  AgentManager mgr(agents_dir_);
  for (int i = 0; i < 3; ++i) {
    mgr.StartScanning(1);
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    mgr.StopScanning();
  }
}

// ======================== Edge cases ========================

TEST_F(AgentManagerTest, ScanNonexistentFilesInDir) {
  AgentManager mgr(agents_dir_);
  // Пустая директория — безопасно
  for (int i = 0; i < 5; ++i) {
    mgr.ScanAndLoad();
  }
  EXPECT_TRUE(mgr.GetAgents().empty());
}

TEST_F(AgentManagerTest, ConcurrentGetAgents) {
  AgentManager mgr(agents_dir_);
  mgr.StartScanning(1);

  std::vector<std::thread> threads;
  for (int i = 0; i < 4; ++i) {
    threads.emplace_back([&mgr]() {
      for (int j = 0; j < 10; ++j) {
        auto agents = mgr.GetAgents();
        (void)agents;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
      }
    });
  }

  for (auto& t : threads) t.join();
  mgr.StopScanning();
}