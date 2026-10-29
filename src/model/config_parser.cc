#include "config_parser.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace s21 {

namespace {

std::string_view Trim(std::string_view str) {
  const auto start = str.find_first_not_of(" \t\r\n");
  if (start == std::string_view::npos) return {};
  const auto end = str.find_last_not_of(" \t\r\n");
  return str.substr(start, end - start + 1);
}

template <typename T>
T ParseNumber(std::string_view str, T default_value = 0) {
  T value;
  auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), value);
  if (ec == std::errc{}) return value;
  return default_value;
}

}  // namespace

AgentConfig ConfigParser::Parse(const std::string& file_path) {
  std::ifstream file(file_path);
  if (!file.is_open()) {
    throw std::runtime_error("Cannot open config: " + file_path);
  }

  AgentConfig config;
  config.config_path = file_path;
  config.type = AgentType::kUnknown;
  config.update_interval_seconds = 5;

  std::string line;
  while (std::getline(file, line)) {
    std::string_view view = Trim(line);
    if (view.empty() || view.starts_with('#')) continue;

    auto delim = view.find('=');
    if (delim == std::string_view::npos) continue;

    std::string_view key = Trim(view.substr(0, delim));
    std::string_view value = Trim(view.substr(delim + 1));

    if (key == "name") {
      config.name = std::string(value);
    } else if (key == "type") {
      if (value == "cpu")
        config.type = AgentType::kCpu;
      else if (value == "memory")
        config.type = AgentType::kMemory;
      else if (value == "network")
        config.type = AgentType::kNetwork;
    } else if (key == "metrics") {
      size_t start = 0;
      while (start < value.size()) {
        auto comma = value.find(',', start);
        auto metric = Trim(value.substr(start, comma - start));
        if (!metric.empty()) {
          config.metrics.emplace_back(metric);
        }
        if (comma == std::string_view::npos) break;
        start = comma + 1;
      }
    } else if (key == "update_interval") {
      config.update_interval_seconds = ParseNumber<int>(value, 5);
    } else if (key == "ping_url") {
      config.ping_url = std::string(value);
    } else if (key.starts_with("critical_")) {
      std::string metric_name{key.substr(9)};

      size_t op_len = 1;
      if (value.size() >= 2 && value[1] == '=') op_len = 2;

      std::string op{value.substr(0, op_len)};
      double val = ParseNumber<double>(value.substr(op_len));

      config.thresholds.push_back({std::move(metric_name), std::move(op), val});
    }
  }

  return config;
}

void ConfigParser::Save(const std::string& file_path,
                        const AgentConfig& config) {
  std::ofstream file(file_path);
  if (!file.is_open()) {
    throw std::runtime_error("Cannot write config: " + file_path);
  }

  file << "name=" << config.name << "\n";
  std::string type_str;
  switch (config.type) {
    case AgentType::kCpu:
      type_str = "cpu";
      break;
    case AgentType::kMemory:
      type_str = "memory";
      break;
    case AgentType::kNetwork:
      type_str = "network";
      break;
    default:
      type_str = "unknown";
      break;
  }
  file << "type=" << type_str << "\n";

  file << "metrics=";
  for (size_t i = 0; i < config.metrics.size(); ++i) {
    if (i > 0) file << ",";
    file << config.metrics[i];
  }
  file << "\n";

  file << "update_interval=" << config.update_interval_seconds << "\n";

  file << "ping_url=" << config.ping_url << "\n";

  for (const auto& th : config.thresholds) {
    file << "critical_" << th.metric_name << "=" << th.op << th.value << "\n";
  }
}

}  // namespace s21
