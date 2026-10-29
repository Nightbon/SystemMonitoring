#include <sys/statvfs.h>

#include <array>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <map>
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

std::map<std::string, double> CalculateCpuUsageByLevel(const CpuTimes& prev,
                                                       const CpuTimes& curr) {
  std::map<std::string, double> result;
  unsigned long total_diff = TotalCpu(curr) - TotalCpu(prev);
  if (total_diff == 0) {
    result["cpu_idle_usage"] = 0.0;
    result["cpu_user_usage"] = 0.0;
    result["cpu_priveleged_usage"] = 0.0;
    result["cpu_dpc_usage"] = 0.0;
    result["cpu_interrupt_usage"] = 0.0;
    return result;
  }

  // idle = idle + iowait
  unsigned long idle_diff =
      (curr.idle + curr.iowait) - (prev.idle + prev.iowait);
  // user = user + nice
  unsigned long user_diff = (curr.user + curr.nice) - (prev.user + prev.nice);
  // priveleged = system + steal
  unsigned long priv_diff =
      (curr.system + curr.steal) - (prev.system + prev.steal);
  // dpc = softirq
  unsigned long dpc_diff = curr.softirq - prev.softirq;
  // interrupt = irq
  unsigned long irq_diff = curr.irq - prev.irq;

  result["cpu_idle_usage"] =
      100.0 * static_cast<double>(idle_diff) / total_diff;
  result["cpu_user_usage"] =
      100.0 * static_cast<double>(user_diff) / total_diff;
  result["cpu_priveleged_usage"] =
      100.0 * static_cast<double>(priv_diff) / total_diff;
  result["cpu_dpc_usage"] = 100.0 * static_cast<double>(dpc_diff) / total_diff;
  result["cpu_interrupt_usage"] =
      100.0 * static_cast<double>(irq_diff) / total_diff;
  return result;
}

// === Swap from /proc/meminfo ===
std::map<std::string, unsigned long> ParseMemInfo() {
  std::map<std::string, unsigned long> info;
  std::ifstream file("/proc/meminfo");
  std::string line;
  while (std::getline(file, line)) {
    std::istringstream iss(line);
    std::string key;
    unsigned long value;
    std::string unit;
    if (iss >> key >> value >> unit) {
      if (!key.empty() && key.back() == ':') key.pop_back();
      info[key] = value;
    }
  }
  return info;
}

// === Process queue length from /proc/loadavg ===
int GetProcQueueLength() {
  std::ifstream file("/proc/loadavg");
  std::string line;
  if (std::getline(file, line)) {
    std::istringstream iss(line);
    double v1, v2, v3;
    int running, total;
    if (iss >> v1 >> v2 >> v3 >> running >> total) {
      // running/total format: "2/123" where total is number of runnable
      // processes
      return total;
    }
  }
  return 0;
}

// === Virtual memory from /proc/meminfo ===
double GetVirtualMemVolume() {
  auto info = ParseMemInfo();
  auto it = info.find("VmallocTotal");
  if (it != info.end()) {
    return static_cast<double>(it->second) / 1024.0;  // MB
  }
  return 0.0;
}

double GetVirtualMemFree() {
  auto info = ParseMemInfo();
  auto it = info.find("VmallocUsed");
  if (it != info.end()) {
    auto total_it = info.find("VmallocTotal");
    if (total_it != info.end()) {
      return static_cast<double>(total_it->second - it->second) / 1024.0;  // MB
    }
  }
  return 0.0;
}

// === Inodes from statvfs ===
int GetInodesTotal() {
  struct statvfs buf;
  if (statvfs("/", &buf) != 0) return 0;
  return static_cast<int>(buf.f_files);
}

// === Disk read time from /proc/diskstats ===
struct DiskStats {
  unsigned long reads = 0;
  unsigned long read_ms = 0;
};

DiskStats ParseDiskStatsLine(const std::string& line) {
  DiskStats stats;
  std::istringstream iss(line);
  int major, minor;
  std::string name;
  iss >> major >> minor >> name;
  if (!name.empty() && std::isdigit(name.back())) return {};  // skip partitions

  unsigned long reads, reads_merged, read_sectors, read_ms;
  unsigned long writes, writes_merged, write_sectors, write_ms;
  unsigned long in_progress, io_ms, weighted_ms;
  iss >> reads >> reads_merged >> read_sectors >> read_ms >> writes >>
      writes_merged >> write_sectors >> write_ms >> in_progress >> io_ms >>
      weighted_ms;
  stats.reads = reads;
  stats.read_ms = read_ms;
  return stats;
}

DiskStats ReadTotalDiskStats() {
  DiskStats total;
  std::ifstream file("/proc/diskstats");
  std::string line;
  while (std::getline(file, line)) {
    auto stats = ParseDiskStatsLine(line);
    total.reads += stats.reads;
    total.read_ms += stats.read_ms;
  }
  return total;
}

double CalculateHardReadTime(const DiskStats& prev, const DiskStats& curr) {
  unsigned long reads_diff = curr.reads - prev.reads;
  unsigned long ms_diff = curr.read_ms - prev.read_ms;
  if (reads_diff == 0) return 0.0;
  return static_cast<double>(ms_diff) /
         static_cast<double>(reads_diff);  // ms per read
}

struct PcloseDeleter {
  void operator()(FILE* f) const {
    if (f) pclose(f);
  }
};

// === System errors from journalctl (last 5 minutes) ===
int CountSystemErrors() {
  // Try journalctl first, fallback to dmesg
  std::array<char, 128> buffer;
  std::string result;
  std::unique_ptr<FILE, PcloseDeleter> pipe(
      popen("journalctl -p err --since '5 minutes ago' --no-pager -q "
            "2>/dev/null | wc -l",
            "r"));
  if (!pipe) return 0;
  while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
    result += buffer.data();
  }
  try {
    return std::stoi(result);
  } catch (...) {
    return 0;
  }
}

// === User auths from last command ===
int CountUserAuths() {
  std::array<char, 128> buffer;
  std::string result;
  std::unique_ptr<FILE, PcloseDeleter> pipe(
      popen("last -s '1 hour ago' 2>/dev/null | grep -v 'wtmp' | wc -l", "r"));
  if (!pipe) return 0;
  while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
    result += buffer.data();
  }
  try {
    return std::stoi(result);
  } catch (...) {
    return 0;
  }
}

class AdditionalAgent : public BaseAgent {
 public:
  explicit AdditionalAgent(const std::string& config_path)
      : BaseAgent(
            config_path, "additional_agent", AgentType::kUnknown,
            {"cpu_idle_usage", "cpu_user_usage", "cpu_priveleged_usage",
             "cpu_dpc_usage", "cpu_interrupt_usage", "total_swap", "used_swap",
             "proc_queue_length", "virtual_mem_volume", "virtual_mem_free",
             "inodes", "hard_read_time", "system_errors", "user_auths"}) {}

  std::vector<Metric> UpdateMetrics(std::stop_token stop_token) override {
    std::vector<Metric> metrics;
    if (stop_token.stop_requested()) return metrics;
    // CPU usage by privilege levels
    auto curr_cpu = ReadCpuStat();
    if (TotalCpu(prev_cpu_) > 0) {
      auto cpu_levels = CalculateCpuUsageByLevel(prev_cpu_, curr_cpu);
      for (const auto& [name, value] : cpu_levels) {
        metrics.push_back({name, value});
      }
    }
    prev_cpu_ = curr_cpu;

    // Swap
    auto meminfo = ParseMemInfo();
    auto swap_total_it = meminfo.find("SwapTotal");
    auto swap_free_it = meminfo.find("SwapFree");
    if (swap_total_it != meminfo.end()) {
      metrics.push_back(
          {"total_swap",
           static_cast<double>(swap_total_it->second) / 1024.0});  // MB
    }
    if (swap_total_it != meminfo.end() && swap_free_it != meminfo.end()) {
      double used =
          static_cast<double>(swap_total_it->second - swap_free_it->second) /
          1024.0;
      metrics.push_back({"used_swap", used});
    }

    if (stop_token.stop_requested()) return metrics;

    // Process queue length
    metrics.push_back({"proc_queue_length", GetProcQueueLength()});

    // Virtual memory
    metrics.push_back({"virtual_mem_volume", GetVirtualMemVolume()});
    metrics.push_back({"virtual_mem_free", GetVirtualMemFree()});

    // Inodes
    metrics.push_back({"inodes", GetInodesTotal()});

    // Hard read time (delta-based)
    auto curr_disk = ReadTotalDiskStats();
    if (prev_disk_.reads > 0) {
      metrics.push_back(
          {"hard_read_time", CalculateHardReadTime(prev_disk_, curr_disk)});
    }
    prev_disk_ = curr_disk;

    if (stop_token.stop_requested()) return metrics;

    // System errors
    metrics.push_back({"system_errors", CountSystemErrors()});

    // User auths
    metrics.push_back({"user_auths", CountUserAuths()});

    return metrics;
  }

 private:
  CpuTimes prev_cpu_;
  DiskStats prev_disk_;
};

}  // namespace s21

extern "C" s21::IAgent* create_agent(const char* config_path) {
  return new s21::AdditionalAgent(config_path);
}
