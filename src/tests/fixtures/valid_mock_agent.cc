#include "../../model/base_agent.h"

namespace s21 {
class MockAgent : public IAgent {
 public:
  std::vector<Metric> UpdateMetrics(std::stop_token stop_token = {}) override {
    return {};
  }
  const AgentConfig& GetConfig() const override {
    static const AgentConfig default_config{};
    return default_config;
  }
  void SetConfig(const AgentConfig& config) override {}
  std::string GetName() const override { return {}; }
  AgentType GetType() const override { return {}; }
  std::time_t GetStartTime() const override { return {}; }
  void SetActive(bool active) override {}
  bool IsActive() const override { return {}; }
  std::chrono::steady_clock::time_point GetLastUpdateTime() const override {
    return {};
  };
  void SetLastUpdateTime(std::chrono::steady_clock::time_point time) override {
  };
};
}  // namespace s21

extern "C" s21::IAgent* create_agent(const char* config_path) {
  return new s21::MockAgent();
}