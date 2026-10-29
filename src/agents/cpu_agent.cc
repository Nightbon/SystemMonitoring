#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "model/base_agent.h"

namespace s21 {

struct CpuTimes {
  unsigned long user = 0;
  unsigned long nice = 0;
  unsigned long system = 0;
  unsigned long idle = 0;
  unsigned long iowait = 0;
  unsigned long irq = 0;
  unsigned long softirq = 0;
  unsigned long steal = 0;
};

CpuTimes ParseCpuTimes(const std::string& line) {
  CpuTimes t;
  std::istringstream iss(line);
  std::string label;
  iss >> label >> t.user >> t.nice >> t.system >> t.idle >> t.iowait >> t.irq >>
      t.softirq >> t.steal;
  return t;
}

unsigned long TotalCpu(const CpuTimes& t) {
  return t.user + t.nice + t.system + t.idle + t.iowait + t.irq + t.softirq +
         t.steal;
}

CpuTimes ReadCpuStat() {
  std::ifstream file("/proc/stat");
  std::string line;
  if (std::getline(file, line) && line.rfind("cpu ", 0) == 0) {
    return ParseCpuTimes(line);
  }
  return {};
}

double CalculateCpuUsage(const CpuTimes& prev, const CpuTimes& curr) {
  unsigned long total_diff = TotalCpu(curr) - TotalCpu(prev);
  unsigned long active_diff = (curr.user + curr.nice + curr.system + curr.irq +
                               curr.softirq + curr.steal) -
                              (prev.user + prev.nice + prev.system + prev.irq +
                               prev.softirq + prev.steal);
  if (total_diff == 0) return 0.0;
  return 100.0 * static_cast<double>(active_diff) /
         static_cast<double>(total_diff);
}

int CountProcesses() {
  int count = 0;
  try {
    for (const auto& entry : std::filesystem::directory_iterator("/proc")) {
      std::string name = entry.path().filename().string();
      if (std::all_of(name.begin(), name.end(), ::isdigit)) {
        ++count;
      }
    }
  } catch (...) {
  }
  return count;
}

class CpuAgent : public BaseAgent {
 public:
  explicit CpuAgent(const std::string& config_path)
      : BaseAgent(config_path, "cpu_agent", AgentType::kCpu,
                  {"cpu", "processes"}) {}

  std::vector<Metric> UpdateMetrics(std::stop_token stop_token) override {
    std::vector<Metric> metrics;
    if (stop_token.stop_requested()) return {};
    auto curr = ReadCpuStat();
    if (TotalCpu(prev_cpu_) > 0) {
      double cpu_pct = CalculateCpuUsage(prev_cpu_, curr);
      metrics.push_back({"cpu", cpu_pct});
    }
    prev_cpu_ = curr;

    metrics.push_back({"processes", CountProcesses()});

    return metrics;
  }

 private:
  CpuTimes prev_cpu_;
};

}  // namespace s21

extern "C" s21::IAgent* create_agent(const char* config_path) {
  return new s21::CpuAgent(config_path);
}
