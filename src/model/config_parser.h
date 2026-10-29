#pragma once
#include <string>

#include "agent_config.h"

namespace s21 {

class ConfigParser {
 public:
  [[nodiscard]] static AgentConfig Parse(const std::string& file_path);
  static void Save(const std::string& file_path, const AgentConfig& config);
};

}  // namespace s21
