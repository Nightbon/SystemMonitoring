#pragma once
#include <chrono>
#include <ctime>
#include <filesystem>
#include <stop_token>
#include <string>
#include <vector>

#include "agent_config.h"
#include "metric.h"

namespace s21 {

namespace chr = std::chrono;
namespace fs = std::filesystem;
using TimePoint = std::chrono::steady_clock::time_point;
using Path = std::filesystem::path;

class IAgent {
 public:
  virtual ~IAgent() = default;
  [[nodiscard]] virtual std::vector<Metric> UpdateMetrics(
      std::stop_token stop_token = {}) = 0;
  [[nodiscard]] virtual const AgentConfig& GetConfig() const = 0;
  virtual void SetConfig(const AgentConfig& config) = 0;
  [[nodiscard]] virtual std::string GetName() const = 0;
  [[nodiscard]] virtual AgentType GetType() const = 0;
  [[nodiscard]] virtual std::time_t GetStartTime() const = 0;
  virtual void SetActive(bool active) = 0;
  [[nodiscard]] virtual bool IsActive() const = 0;
  [[nodiscard]] virtual TimePoint GetLastUpdateTime() const = 0;
  virtual void SetLastUpdateTime(TimePoint time) = 0;
};

}  // namespace s21
