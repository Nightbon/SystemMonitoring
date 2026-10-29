#pragma once
#include <filesystem>
#include <format>
#include <fstream>
#include <mutex>
#include <shared_mutex>
#include <span>
#include <string>
#include <vector>

#include "metric.h"

namespace s21 {

namespace fs = std::filesystem;
using Path = std::filesystem::path;

class Logger {
 public:
  explicit Logger(Path logs_dir);
  ~Logger();

  void LogMetrics(std::span<const Metric> metrics);
  [[nodiscard]] std::vector<std::string> GetLastLines(int count) const;

 private:
  Path logs_dir_;
  std::string current_date_;
  std::ofstream file_stream_;
  mutable std::shared_mutex mutex_;

  [[nodiscard]] std::string GetCurrentDate() const;
  [[nodiscard]] std::string GetCurrentTimestamp() const;
  void OpenFileForDate(const std::string& date);
};

}  // namespace s21
