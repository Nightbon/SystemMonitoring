#pragma once
#include <ctime>
#include <string>
#include <vector>

#include "agent_config.h"
#include "i_agent.h"

namespace s21 {

class BaseAgent : public IAgent {
 public:
  BaseAgent(const std::string& config_path, const std::string& default_name,
            AgentType default_type, std::vector<std::string> default_metrics);

  const AgentConfig& GetConfig() const override { return config_; }
  void SetConfig(const AgentConfig& config) override { config_ = config; }
  std::string GetName() const override { return config_.name; }
  AgentType GetType() const override { return config_.type; }
  std::time_t GetStartTime() const override { return start_time_; }
  void SetActive(bool active) override { active_ = active; }
  bool IsActive() const override { return active_; }
  TimePoint GetLastUpdateTime() const override { return last_update_time_; }
  void SetLastUpdateTime(TimePoint time) override { last_update_time_ = time; }

 protected:
  TimePoint last_update_time_ = TimePoint::min();
  AgentConfig config_;
  std::time_t start_time_;
  std::atomic<bool> active_;
};

}  // namespace s21
