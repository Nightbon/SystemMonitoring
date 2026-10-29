#pragma once

#include <filesystem>
#include <memory>
#include <stop_token>

#include "i_agent.h"

namespace s21 {

struct LoadedAgent {
  std::shared_ptr<IAgent> agent;
  Path so_path;
};

class AgentLoader {
 public:
  [[nodiscard]] LoadedAgent Load(const Path& so_path, const Path& config_path,
                                 std::stop_token stop_token = {});
};

}  // namespace s21
