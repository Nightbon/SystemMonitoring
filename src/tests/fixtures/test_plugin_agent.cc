// Тестовый плагин-агент для unit-тестов MonitorCore.
// Компилируется как .so и загружается через dlopen AgentManager'ом.
// Управляется через файл /tmp/s21_plugin_control.txt:
//   "cpu=95.0,ram=85.0"  — вернуть указанные метрики
//   "throw"               — бросить исключение в UpdateMetrics
//   "empty"               — вернуть пустой вектор
//   файл отсутствует      — вернуть {{"cpu", 50.0}} по умолчанию

#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "../../model/base_agent.h"
#include "../../model/metric.h"

static constexpr auto kControlPath = "/tmp/s21_plugin_control.txt";

class TestPluginAgent : public s21::BaseAgent {
 public:
  explicit TestPluginAgent(const std::string& config_path)
      : s21::BaseAgent(config_path, "test_plugin", s21::AgentType::kCpu,
                       {"cpu", "ram"}) {}

  std::vector<s21::Metric> UpdateMetrics(std::stop_token stop_token) override {
    if (stop_token.stop_requested()) return {};
    std::ifstream f(kControlPath);
    if (!f.is_open()) return {{"cpu", 50.0}};

    std::string cmd;
    std::getline(f, cmd);

    if (cmd == "throw") {
      throw std::runtime_error("test exception from plugin");
    }
    if (cmd == "empty") {
      return {};
    }

    // Парсинг "cpu=95.0,ram=85.0"
    std::vector<s21::Metric> metrics;
    std::istringstream iss(cmd);
    std::string pair;
    while (std::getline(iss, pair, ',')) {
      auto eq = pair.find('=');
      if (eq == std::string::npos) continue;
      std::string name = pair.substr(0, eq);
      double val = std::stod(pair.substr(eq + 1));
      metrics.push_back({name, val});
    }
    return metrics;
  }
};

extern "C" {
s21::IAgent* create_agent(const char* config_path) {
  return new TestPluginAgent(config_path);
}
}