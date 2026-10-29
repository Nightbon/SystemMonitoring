#include "logger.h"

#include <chrono>
#include <filesystem>

namespace s21 {

namespace {

void EnsureLogsDir(const Path& logs_dir) {
  if (fs::exists(logs_dir)) {
    if (fs::is_regular_file(logs_dir) || !fs::is_directory(logs_dir)) {
      fs::remove(logs_dir);
    }
  }
  fs::create_directories(logs_dir);
}

}  // namespace

Logger::Logger(Path logs_dir) : logs_dir_(std::move(logs_dir)) {
  EnsureLogsDir(logs_dir_);
  current_date_ = GetCurrentDate();
  OpenFileForDate(current_date_);
}

Logger::~Logger() {
  if (file_stream_.is_open()) {
    file_stream_.close();
  }
}

void Logger::LogMetrics(std::span<const Metric> metrics) {
  try {
    std::unique_lock lock(mutex_);

    std::string date = GetCurrentDate();
    if (date != current_date_) {
      current_date_ = date;
      OpenFileForDate(current_date_);
    }

    if (!file_stream_.is_open()) return;

    std::string line = std::format("[{}] | ", GetCurrentTimestamp());
    for (size_t i = 0; i < metrics.size(); ++i) {
      if (i > 0) line += " | ";
      line += std::format("{} : ", metrics[i].name);
      std::visit(
          [&line](auto&& arg) {
            using T = std::remove_cvref_t<decltype(arg)>;

            if constexpr (std::is_floating_point_v<T>) {
              line += std::format("{:.2f}", arg);
            } else {
              line += std::format("{}", arg);
            }
          },
          metrics[i].value);
    }

    file_stream_ << line << '\n';
    file_stream_.flush();
  } catch (...) {
    // Silently ignore logging errors
  }
}

std::vector<std::string> Logger::GetLastLines(int count) const {
  std::shared_lock lock(mutex_);

  std::string file_path = (logs_dir_ / (current_date_ + ".log")).string();
  std::ifstream file(file_path);
  if (!file.is_open()) return {};

  std::vector<std::string> result;
  std::string line;
  while (std::getline(file, line)) {
    result.push_back(std::move(line));
    if (result.size() > static_cast<size_t>(count)) {
      result.erase(result.begin());
    }
  }
  return result;
}

std::string Logger::GetCurrentDate() const {
  auto now = std::chrono::system_clock::now();
  auto local_time = std::chrono::current_zone()->to_local(now);
  return std::format("{:%Y-%m-%d}", local_time);
}

std::string Logger::GetCurrentTimestamp() const {
  auto now = std::chrono::system_clock::now();
  auto local_time = std::chrono::current_zone()->to_local(now);
  auto local_sec = std::chrono::floor<std::chrono::seconds>(local_time);
  return std::format("{:%Y-%m-%d %H:%M:%S}", local_sec);
}

void Logger::OpenFileForDate(const std::string& date) {
  if (file_stream_.is_open()) {
    file_stream_.close();
  }
  auto file_path = logs_dir_ / (date + ".log");
  file_stream_.open(file_path, std::ios::app);
  if (!file_stream_.is_open()) {
    EnsureLogsDir(logs_dir_);
    file_stream_.open(file_path, std::ios::app);
  }
}

}  // namespace s21
