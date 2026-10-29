#pragma once
#include <compare>
#include <string>
#include <vector>

namespace s21 {

enum class AgentType { kCpu, kMemory, kNetwork, kUnknown };

struct CriticalThreshold {
  std::string metric_name;
  std::string op;
  double value = 0.0;

  auto operator<=>(const CriticalThreshold&) const = default;
};

struct AgentConfig {
  std::string name;
  AgentType type = AgentType::kUnknown;
  std::vector<std::string> metrics;
  std::vector<CriticalThreshold> thresholds;
  int update_interval_seconds = 5;
  std::string config_path;
  std::string ping_url = "google.com";

  auto operator<=>(const AgentConfig&) const = default;
};

constexpr const char* AgentTypeToString(AgentType type) noexcept {
  switch (type) {
    case AgentType::kCpu:
      return "CPU";
    case AgentType::kMemory:
      return "Memory";
    case AgentType::kNetwork:
      return "Network";
    default:
      return "Unknown";
  }
}

constexpr int ScanAgentsInterval = 5;

}  // namespace s21
