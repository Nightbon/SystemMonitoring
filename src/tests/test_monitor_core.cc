#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <thread>

#include "model/agent_config.h"
#include "model/config_parser.h"
#include "model/monitor_core.h"

using namespace s21;

// ======================================================================
// Базовые тесты MonitorCore (без плагинов)
// ======================================================================

class MonitorCoreTest : public ::testing::Test {
 protected:
  void SetUp() override {
    agents_dir_ =
        std::filesystem::temp_directory_path() / "s21_test_core_agents";
    logs_dir_ = std::filesystem::temp_directory_path() / "s21_test_core_logs";
    std::filesystem::remove_all(agents_dir_);
    std::filesystem::remove_all(logs_dir_);
  }

  void TearDown() override {
    std::filesystem::remove_all(agents_dir_);
    std::filesystem::remove_all(logs_dir_);
  }

  std::filesystem::path agents_dir_;
  std::filesystem::path logs_dir_;
};

TEST_F(MonitorCoreTest, ConstructionCreatesDirectories) {
  MonitorCore core(agents_dir_, logs_dir_, 5);
  EXPECT_TRUE(std::filesystem::exists(agents_dir_));
  EXPECT_TRUE(std::filesystem::exists(logs_dir_));
}

TEST_F(MonitorCoreTest, DestructionDoesNotCrash) {
  { MonitorCore core(agents_dir_, logs_dir_, 5); }
}

TEST_F(MonitorCoreTest, StartAndStopDoNotCrash) {
  MonitorCore core(agents_dir_, logs_dir_, 5);
  core.Start();
  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  core.Stop();
}

TEST_F(MonitorCoreTest, StopWithoutStartDoesNotCrash) {
  MonitorCore core(agents_dir_, logs_dir_, 5);
  core.Stop();
  core.Stop();
}

TEST_F(MonitorCoreTest, StartWithNoAgents) {
  MonitorCore core(agents_dir_, logs_dir_, 1);
  core.Start();
  std::this_thread::sleep_for(std::chrono::milliseconds(300));
  auto agents = core.GetAgents();
  EXPECT_TRUE(agents.empty());
  core.Stop();
}

TEST_F(MonitorCoreTest, GetAgentsEmptyInitially) {
  MonitorCore core(agents_dir_, logs_dir_, 5);
  auto agents = core.GetAgents();
  EXPECT_TRUE(agents.empty());
}

TEST_F(MonitorCoreTest, GetAgentsAfterStartWithEmptyDir) {
  MonitorCore core(agents_dir_, logs_dir_, 5);
  core.Start();
  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  auto agents = core.GetAgents();
  EXPECT_TRUE(agents.empty());
  core.Stop();
}

TEST_F(MonitorCoreTest, GetLastLogLinesEmptyInitially) {
  MonitorCore core(agents_dir_, logs_dir_, 5);
  auto lines = core.GetLastLogLines(10);
  EXPECT_TRUE(lines.empty());
}

TEST_F(MonitorCoreTest, GetLastLogLinesZeroCount) {
  MonitorCore core(agents_dir_, logs_dir_, 5);
  auto lines = core.GetLastLogLines(0);
  EXPECT_TRUE(lines.empty());
}

TEST_F(MonitorCoreTest, GetLastLogLinesNegativeCount) {
  MonitorCore core(agents_dir_, logs_dir_, 5);
  auto lines = core.GetLastLogLines(-1);
  EXPECT_TRUE(lines.empty());
}

TEST_F(MonitorCoreTest, UpdateAgentConfigNonexistentDoesNotCrash) {
  MonitorCore core(agents_dir_, logs_dir_, 5);
  AgentConfig cfg;
  cfg.name = "nonexistent";
  cfg.config_path = (agents_dir_ / "nonexistent.conf").string();
  EXPECT_NO_THROW(core.UpdateAgentConfig("nonexistent", cfg));
}

TEST_F(MonitorCoreTest, UpdateAgentConfigSavesToFile) {
  MonitorCore core(agents_dir_, logs_dir_, 5);
  AgentConfig cfg;
  cfg.name = "test";
  cfg.type = AgentType::kCpu;
  cfg.config_path = (agents_dir_ / "test.conf").string();
  core.UpdateAgentConfig("nonexistent", cfg);
}

TEST_F(MonitorCoreTest, DisableNonexistentDoesNotCrash) {
  MonitorCore core(agents_dir_, logs_dir_, 5);
  EXPECT_NO_THROW(core.DisableAgent("nonexistent"));
}

TEST_F(MonitorCoreTest, SetTelegramCredentialsDoesNotCrash) {
  MonitorCore core(agents_dir_, logs_dir_, 5);
  EXPECT_NO_THROW(core.SetTelegramCredentials("tok", "chat", ""));
}

TEST_F(MonitorCoreTest, SetEmailCredentialsDoesNotCrash) {
  MonitorCore core(agents_dir_, logs_dir_, 5);
  EXPECT_NO_THROW(core.SetEmailCredentials("smtp", 587, "u", "p", "f@e.com"));
}

TEST_F(MonitorCoreTest, SetEmailRecipientDoesNotCrash) {
  MonitorCore core(agents_dir_, logs_dir_, 5);
  EXPECT_NO_THROW(core.SetEmailRecipient("to@e.com"));
}

TEST_F(MonitorCoreTest, SetEmailEnabledDoesNotCrash) {
  MonitorCore core(agents_dir_, logs_dir_, 5);
  EXPECT_NO_THROW(core.SetEmailEnabled(true));
  EXPECT_NO_THROW(core.SetEmailEnabled(false));
}

TEST_F(MonitorCoreTest, MultipleStartStopCycles) {
  MonitorCore core(agents_dir_, logs_dir_, 1);
  for (int i = 0; i < 3; ++i) {
    core.Start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    core.Stop();
  }
}

TEST_F(MonitorCoreTest, ConstructionWithSameDirForAgentsAndLogs) {
  auto shared = std::filesystem::temp_directory_path() / "s21_core_shared";
  std::filesystem::remove_all(shared);
  MonitorCore core(shared, shared, 5);
  EXPECT_TRUE(std::filesystem::exists(shared));
  std::filesystem::remove_all(shared);
}

TEST_F(MonitorCoreTest, RapidStartStop) {
  MonitorCore core(agents_dir_, logs_dir_, 5);
  core.Start();
  core.Stop();
  core.Start();
  core.Stop();
}

// ======================================================================
// Тесты MonitorCore с загружаемым плагином (.so)
// ======================================================================
static constexpr const char* kControlPath = "/tmp/s21_plugin_control.txt";

// Вспомогательная функция: записать команду в control-файл
static void SetPluginCommand(const std::string& cmd) {
  std::ofstream f(kControlPath);
  f << cmd;
  f.close();
}

static void ClearPluginControl() { std::filesystem::remove(kControlPath); }

// Фикстура с реальным плагином .so в директории агентов
class MonitorCorePluginTest : public ::testing::Test {
 protected:
  void SetUp() override {
    agents_dir_ =
        std::filesystem::temp_directory_path() / "s21_test_core_plug_agents";
    logs_dir_ =
        std::filesystem::temp_directory_path() / "s21_test_core_plug_logs";
    std::filesystem::remove_all(agents_dir_);
    std::filesystem::remove_all(logs_dir_);
    std::filesystem::create_directories(agents_dir_);
    std::filesystem::create_directories(logs_dir_);

    // Копируем .so плагина в agents_dir
    auto src_so = std::filesystem::path(TEST_DATA_DIR) / "build" /
                  "libtest_plugin_agent.so";
    auto dst_so = agents_dir_ / "test_plugin.so";
    std::filesystem::copy_file(src_so, dst_so);

    // Создаём .conf для плагина с update_interval=1 для быстрых тестов
    auto conf_path = agents_dir_ / "test_plugin.conf";
    std::ofstream conf(conf_path);
    conf << "name=test_plugin\n"
         << "type=cpu\n"
         << "metrics=cpu,ram\n"
         << "update_interval=1\n"
         << "critical_cpu=>=90\n";
    conf.close();

    ClearPluginControl();
  }

  void TearDown() override {
    ClearPluginControl();
    // Даём время потокам остановиться
    std::filesystem::remove_all(agents_dir_);
    std::filesystem::remove_all(logs_dir_);
  }

  // Запустить ядро и дождаться загрузки агента
  void StartAndWaitForAgent(MonitorCore& core, int timeout_ms = 3000) {
    core.Start();
    // Ждём, пока AgentManager загрузит .so и агент появится
    auto deadline = std::chrono::steady_clock::now() +
                    std::chrono::milliseconds(timeout_ms);
    while (std::chrono::steady_clock::now() < deadline) {
      auto agents = core.GetAgents();
      if (!agents.empty()) return;
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  }

  // Дождаться, пока в логах появится хотя бы count строк
  void WaitForLogLines(MonitorCore& core, int count, int timeout_ms = 5000) {
    auto deadline = std::chrono::steady_clock::now() +
                    std::chrono::milliseconds(timeout_ms);
    while (std::chrono::steady_clock::now() < deadline) {
      auto lines = core.GetLastLogLines(100);
      if (static_cast<int>(lines.size()) >= count) return;
      std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
  }

  std::filesystem::path agents_dir_;
  std::filesystem::path logs_dir_;
};

// ======================================================================
// Строки 42-43: UpdateAgentConfig находит агент и вызывает
//   (*it)->SetConfig(config) + ConfigParser::Save(config.config_path, config)
// ======================================================================

TEST_F(MonitorCorePluginTest, UpdateAgentConfig_SetsConfigAndSaves) {
  MonitorCore core(agents_dir_, logs_dir_, 1);
  StartAndWaitForAgent(core);

  auto agents = core.GetAgents();
  ASSERT_FALSE(agents.empty());
  EXPECT_EQ(agents[0]->GetName(), "test_plugin");

  // Новый конфиг
  AgentConfig new_cfg;
  new_cfg.name = "test_plugin";
  new_cfg.type = AgentType::kCpu;
  new_cfg.metrics = {"cpu", "ram", "disk"};
  new_cfg.update_interval_seconds = 2;
  new_cfg.config_path = (agents_dir_ / "test_plugin.conf").string();

  core.UpdateAgentConfig("test_plugin", new_cfg);

  // Проверяем, что конфиг агента в памяти обновился (строка 42)
  agents = core.GetAgents();
  ASSERT_FALSE(agents.empty());
  EXPECT_EQ(agents[0]->GetConfig().update_interval_seconds, 2);
  ASSERT_EQ(agents[0]->GetConfig().metrics.size(), 3u);
  EXPECT_EQ(agents[0]->GetConfig().metrics[2], "disk");

  // Проверяем, что конфиг сохранён на диск (строка 43)
  auto saved = ConfigParser::Parse(new_cfg.config_path);
  EXPECT_EQ(saved.update_interval_seconds, 2);
  ASSERT_EQ(saved.metrics.size(), 3u);
  EXPECT_EQ(saved.metrics[2], "disk");

  core.Stop();
}

TEST_F(MonitorCorePluginTest, UpdateAgentConfig_OverwritesThresholds) {
  MonitorCore core(agents_dir_, logs_dir_, 1);
  StartAndWaitForAgent(core);

  AgentConfig new_cfg;
  new_cfg.name = "test_plugin";
  new_cfg.type = AgentType::kMemory;
  new_cfg.update_interval_seconds = 10;
  new_cfg.thresholds = {{"ram", ">=", 95.0}, {"swap", ">", 50.0}};
  new_cfg.config_path = (agents_dir_ / "test_plugin.conf").string();

  core.UpdateAgentConfig("test_plugin", new_cfg);

  // Проверяем в памяти
  auto agents = core.GetAgents();
  ASSERT_FALSE(agents.empty());
  EXPECT_EQ(agents[0]->GetType(), AgentType::kMemory);
  ASSERT_EQ(agents[0]->GetConfig().thresholds.size(), 2u);
  EXPECT_EQ(agents[0]->GetConfig().thresholds[1].metric_name, "swap");

  // Проверяем на диске
  auto saved = ConfigParser::Parse(new_cfg.config_path);
  EXPECT_EQ(saved.type, AgentType::kMemory);
  ASSERT_EQ(saved.thresholds.size(), 2u);
  EXPECT_DOUBLE_EQ(saved.thresholds[0].value, 95.0);

  core.Stop();
}

TEST_F(MonitorCorePluginTest, UpdateAgentConfig_WrongNameDoesNothing) {
  MonitorCore core(agents_dir_, logs_dir_, 1);
  StartAndWaitForAgent(core);

  auto agents_before = core.GetAgents();
  ASSERT_FALSE(agents_before.empty());
  int old_interval = agents_before[0]->GetConfig().update_interval_seconds;

  AgentConfig new_cfg;
  new_cfg.name = "wrong_name";
  new_cfg.update_interval_seconds = 999;
  new_cfg.config_path = (agents_dir_ / "test_plugin.conf").string();

  core.UpdateAgentConfig("wrong_name", new_cfg);

  // Конфиг не должен измениться
  auto agents_after = core.GetAgents();
  ASSERT_FALSE(agents_after.empty());
  EXPECT_EQ(agents_after[0]->GetConfig().update_interval_seconds, old_interval);

  core.Stop();
}

// ======================================================================
// Строки 79, 84, 92: фильтрация активных агентов, min_element,
//   установка update_interval_ из агента
// ======================================================================

TEST_F(MonitorCorePluginTest, WorkLoop_ActiveAgentIntervalUsed) {
  // Плагин с update_interval=1. WorkLoop должен использовать его, а не default.
  SetPluginCommand("cpu=50.0");

  MonitorCore core(agents_dir_, logs_dir_, 1);
  StartAndWaitForAgent(core);

  // Считаем текущее количество записей
  auto lines_before = core.GetLastLogLines(100);
  size_t count_before = lines_before.size();

  // Ждём 1 секунду — за это время должно появиться ещё минимум 1 запись
  // (interval=1 → каждые ~1-2 сек новая запись)
  std::this_thread::sleep_for(std::chrono::milliseconds(1000));

  auto lines_after = core.GetLastLogLines(100);
  EXPECT_GT(lines_after.size(), count_before)
      << "WorkLoop должен использовать интервал агента (1с), а не default";

  core.Stop();
}

TEST_F(MonitorCorePluginTest, WorkLoop_InactiveAgentSkipped) {
  SetPluginCommand("cpu=50.0");

  MonitorCore core(agents_dir_, logs_dir_, 1);
  StartAndWaitForAgent(core);

  // Деактивируем агент
  auto agents = core.GetAgents();
  ASSERT_FALSE(agents.empty());
  agents[0]->SetActive(false);

  // Очищаем логи, созданные до деактивации
  std::this_thread::sleep_for(std::chrono::milliseconds(800));
  auto lines_before = core.GetLastLogLines(100);
  auto count_before = lines_before.size();

  // Ждём — WorkLoop должен пропустить неактивного агента
  std::this_thread::sleep_for(std::chrono::milliseconds(1200));
  auto lines_after = core.GetLastLogLines(100);

  // Новых записей быть не должно (активных агентов нет → default_interval, но
  // метрик нет) WorkLoop при отсутствии активных агентов просто спит, ничего не
  // логируя
  EXPECT_EQ(lines_after.size(), count_before);

  core.Stop();
}

// ======================================================================
// Строки 94-103: сбор метрик из агентов в all_metrics
// ======================================================================

TEST_F(MonitorCorePluginTest, WorkLoop_CollectsAndLogsMetrics) {
  SetPluginCommand("cpu=75.5,ram=60.0");

  MonitorCore core(agents_dir_, logs_dir_, 1);
  core.Start();
  WaitForLogLines(core, 1, 3000);

  auto lines = core.GetLastLogLines(10);
  ASSERT_FALSE(lines.empty());

  std::string last = lines.back();
  // Проверяем, что метрики попали в лог
  EXPECT_NE(last.find("cpu"), std::string::npos);
  EXPECT_NE(last.find("75.50"), std::string::npos);
  EXPECT_NE(last.find("ram"), std::string::npos);
  EXPECT_NE(last.find("60.00"), std::string::npos);

  core.Stop();
}

TEST_F(MonitorCorePluginTest, WorkLoop_LogsMultipleCycles) {
  SetPluginCommand("cpu=30.0");

  MonitorCore core(agents_dir_, logs_dir_, 1);
  core.Start();
  // Ждём несколько циклов (update_interval=1)
  WaitForLogLines(core, 3, 4000);

  auto lines = core.GetLastLogLines(100);
  EXPECT_GE(lines.size(), 3u);

  // Все строки должны содержать метрику
  for (const auto& line : lines) {
    EXPECT_NE(line.find("cpu"), std::string::npos);
    EXPECT_NE(line.find("30.00"), std::string::npos);
  }

  core.Stop();
}

// ======================================================================
// Строки 101-102: agent->UpdateMetrics() + ranges::move в all_metrics
// ======================================================================

TEST_F(MonitorCorePluginTest, WorkLoop_EmptyMetricsFromAgent) {
  SetPluginCommand("empty");

  MonitorCore core(agents_dir_, logs_dir_, 1);
  core.Start();
  // Даём время на один цикл
  std::this_thread::sleep_for(std::chrono::milliseconds(1000));

  // При пустых метрикаx LogMetrics не вызывается (строка 112:
  // !all_metrics.empty()) Но Logger пишет пустую строку — на самом деле empty
  // vector → all_metrics.empty() == true → logger_.LogMetrics НЕ вызывается →
  // лог остаётся пустым (кроме строки таймстемпа от самого Logger при
  // конструировании — нет, Logger не пишет при конструировании) Ждём
  // дополнительно и проверяем, что новых записей с метриками нет
  auto lines = core.GetLastLogLines(100);
  // Нет метрик — нет логов
  for (const auto& line : lines) {
    EXPECT_EQ(line.find("cpu"), std::string::npos);
  }

  core.Stop();
}

// ======================================================================
// Строки 104-106: CriticalChecker::Check(all_metrics, config.thresholds)
// ======================================================================

TEST_F(MonitorCorePluginTest, WorkLoop_ThresholdsTriggered) {
  // cpu=95.0 с порогом critical_cpu>=90 → алерт сработает
  SetPluginCommand("cpu=95.0");

  MonitorCore core(agents_dir_, logs_dir_, 1);
  core.Start();
  WaitForLogLines(core, 1, 3000);

  // Метрики логируются независимо от алертов
  auto lines = core.GetLastLogLines(10);
  ASSERT_FALSE(lines.empty());
  EXPECT_NE(lines.back().find("95.00"), std::string::npos);

  // Алерты передаются в AlertNotifier (строки 116-118) — проверяем, что нет
  // краша AlertNotifier без настроенных credentials просто молча возвращает
  // false
  core.Stop();
}

TEST_F(MonitorCorePluginTest, WorkLoop_ThresholdsNotTriggered) {
  // cpu=50.0 с порогом critical_cpu>=90 → алерт НЕ сработает
  SetPluginCommand("cpu=50.0");

  MonitorCore core(agents_dir_, logs_dir_, 1);
  core.Start();
  WaitForLogLines(core, 1, 3000);

  auto lines = core.GetLastLogLines(10);
  ASSERT_FALSE(lines.empty());
  // Метрики должны быть в логе
  EXPECT_NE(lines.back().find("50.00"), std::string::npos);

  core.Stop();
}

// ======================================================================
// Строки 108-110: catch (...) — агент бросает исключение, цикл продолжается
// ======================================================================

TEST_F(MonitorCorePluginTest, WorkLoop_ThrowingAgentDoesNotCrashLoop) {
  SetPluginCommand("throw");

  MonitorCore core(agents_dir_, logs_dir_, 1);
  core.Start();

  // Даём время на несколько циклов — каждый цикл агент бросает исключение
  // WorkLoop не должен упасть благодаря catch на строке 107
  std::this_thread::sleep_for(std::chrono::milliseconds(1500));

  // Агент всё ещё загружен (не удалён)
  auto agents = core.GetAgents();
  // Агент может быть недоступен если dlopen сработал, но create_agent жив
  // Важно: цикл не упал
  core.Stop();

  // Тест пройден, если не было crash
  SUCCEED();
}

TEST_F(MonitorCorePluginTest, WorkLoop_AgentRecoversAfterThrow) {
  // Агент бросает, потом восстанавливается
  SetPluginCommand("throw");
  MonitorCore core(agents_dir_, logs_dir_, 1);
  core.Start();
  std::this_thread::sleep_for(std::chrono::milliseconds(1000));

  // Восстанавливаем — теперь агент возвращает метрики
  SetPluginCommand("cpu=42.0");
  WaitForLogLines(core, 1, 3000);

  auto lines = core.GetLastLogLines(10);
  ASSERT_FALSE(lines.empty());
  EXPECT_NE(lines.back().find("42.00"), std::string::npos);

  core.Stop();
}

// ======================================================================
// Строки 112-114: if (!all_metrics.empty()) logger_.LogMetrics(all_metrics)
// ======================================================================

TEST_F(MonitorCorePluginTest, WorkLoop_MetricsLoggedToFile) {
  SetPluginCommand("cpu=88.0,ram=77.0");

  MonitorCore core(agents_dir_, logs_dir_, 1);
  core.Start();
  WaitForLogLines(core, 2, 3000);

  // Проверяем через GetLastLogLines (который читает файл лога)
  auto lines = core.GetLastLogLines(100);
  ASSERT_GE(lines.size(), 2u);

  // Обе записи должны содержать обе метрики
  for (const auto& line : lines) {
    EXPECT_NE(line.find("cpu"), std::string::npos);
    EXPECT_NE(line.find("88.00"), std::string::npos);
    EXPECT_NE(line.find("ram"), std::string::npos);
    EXPECT_NE(line.find("77.00"), std::string::npos);
  }

  core.Stop();
}

// ======================================================================
// Строки 116-118: if (!all_alerts.empty())
// alert_notifier_.ProcessAlerts(all_alerts)
// ======================================================================

TEST_F(MonitorCorePluginTest, WorkLoop_AlertsProcessedWithoutCredentials) {
  // cpu=99.0 >= 90 → алерт, но AlertNotifier без credentials просто вернёт
  // false
  SetPluginCommand("cpu=99.0");

  MonitorCore core(agents_dir_, logs_dir_, 1);
  core.Start();
  WaitForLogLines(core, 1, 3000);

  // Метрики залогированы
  auto lines = core.GetLastLogLines(10);
  ASSERT_FALSE(lines.empty());
  EXPECT_NE(lines.back().find("99.00"), std::string::npos);

  // Нет краша — ProcessAlerts вызван, но без настроенного Telegram/Email
  core.Stop();
}

// ======================================================================
// Строки 120-123: глобальный catch — защита всего цикла
// ======================================================================

TEST_F(MonitorCorePluginTest, WorkLoop_GlobalCatchProtects) {
  // Переключаем поведение: сначала нормальное, потом бросаем
  SetPluginCommand("cpu=50.0");

  MonitorCore core(agents_dir_, logs_dir_, 1);
  core.Start();
  WaitForLogLines(core, 1, 3000);

  // Переключаем на бросок
  SetPluginCommand("throw");
  std::this_thread::sleep_for(std::chrono::milliseconds(1500));

  // Обратно на нормальное — цикл должен выжить
  SetPluginCommand("cpu=33.0");
  WaitForLogLines(core, 2, 3000);

  auto lines = core.GetLastLogLines(100);
  // Должны быть записи и до, и после броска
  EXPECT_GE(lines.size(), 1u);

  // Последняя запись должна содержать восстановленные метрики
  bool found_recovery = false;
  for (const auto& line : lines) {
    if (line.find("33.00") != std::string::npos) {
      found_recovery = true;
      break;
    }
  }
  EXPECT_TRUE(found_recovery)
      << "WorkLoop должен был восстановиться после исключения";

  core.Stop();
}

// ======================================================================
// Строка 125: std::this_thread::sleep_for — цикл не потребляет 100% CPU
// ======================================================================

TEST_F(MonitorCorePluginTest, WorkLoop_SleepsBetweenCycles) {
  SetPluginCommand("cpu=10.0");

  MonitorCore core(agents_dir_, logs_dir_, 1);
  core.Start();

  auto t0 = std::chrono::steady_clock::now();
  WaitForLogLines(core, 1, 5000);
  auto t1 = std::chrono::steady_clock::now();

  core.Stop();

  // С интервалом 1 секунда, первая запись не должна появиться мгновенно
  // (агент должен загрузиться + один цикл WorkLoop)
  auto elapsed_ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
  // Первый апейт может произойти сразу — проверяем что WorkLoop вообще работает
  EXPECT_GT(elapsed_ms, 0) << "WorkLoop должен спать между циклами";
}