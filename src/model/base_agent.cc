#include "base_agent.h"

#include "config_parser.h"

namespace s21 {

BaseAgent::BaseAgent(const std::string& config_path,
                     const std::string& default_name, AgentType default_type,
                     std::vector<std::string> default_metrics)
    : start_time_(std::time(nullptr)), active_(true) {
  try {
    config_ = ConfigParser::Parse(config_path);
  } catch (...) {
    config_.name = default_name;
    config_.type = default_type;
    config_.metrics = std::move(default_metrics);
    config_.update_interval_seconds = 5;
    config_.config_path = config_path;
  }
}

}  // namespace s21
