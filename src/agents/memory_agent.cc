#include <sys/statvfs.h>

#include <chrono>
#include <cstdio>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "model/base_agent.h"

namespace s21 {

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
      // Remove trailing ':' from key
      if (!key.empty() && key.back() == ':') key.pop_back();
      info[key] = value;
    }
  }
  return info;
}

double GetRamUsagePercent(const std::map<std::string, unsigned long>& info) {
  auto total_it = info.find("MemTotal");
  auto avail_it = info.find("MemAvailable");
  if (total_it == info.end() || avail_it == info.end()) return 0.0;
  unsigned long total = total_it->second;
  unsigned long avail = avail_it->second;
  if (total == 0) return 0.0;
  return 100.0 *
         (1.0 - static_cast<double>(avail) / static_cast<double>(total));
}

struct DiskStats {
  unsigned long reads = 0;
  unsigned long writes = 0;
  unsigned long read_sectors = 0;
  unsigned long write_sectors = 0;
  unsigned long read_ms = 0;
  unsigned long write_ms = 0;
  unsigned long io_ms = 0;
};

DiskStats ParseDiskStatsLine(const std::string& line) {
  DiskStats stats;
  std::istringstream iss(line);
  int major, minor;
  std::string name;
  iss >> major >> minor >> name;
  // Skip to field 4 (reads)
  unsigned long reads, reads_merged, read_sectors, read_ms;
  unsigned long writes, writes_merged, write_sectors, write_ms;
  unsigned long in_progress, io_ms, weighted_ms;
  iss >> reads >> reads_merged >> read_sectors >> read_ms >> writes >>
      writes_merged >> write_sectors >> write_ms >> in_progress >> io_ms >>
      weighted_ms;
  stats.reads = reads;
  stats.writes = writes;
  stats.read_sectors = read_sectors;
  stats.write_sectors = write_sectors;
  stats.read_ms = read_ms;
  stats.write_ms = write_ms;
  stats.io_ms = io_ms;
  return stats;
}

DiskStats ReadTotalDiskStats() {
  DiskStats total;
  std::ifstream file("/proc/diskstats");
  std::string line;
  while (std::getline(file, line)) {
    std::istringstream iss(line);
    int major, minor;
    std::string name;
    iss >> major >> minor >> name;
    // Skip partitions (minor != 0 usually means partition, but we want whole
    // disks) Whole disks typically have names like sda, nvme0n1, vda (no digits
    // at end)
    if (!name.empty() && std::isdigit(name.back())) continue;

    auto stats = ParseDiskStatsLine(line);
    total.reads += stats.reads;
    total.writes += stats.writes;
    total.read_sectors += stats.read_sectors;
    total.write_sectors += stats.write_sectors;
    total.read_ms += stats.read_ms;
    total.write_ms += stats.write_ms;
    total.io_ms += stats.io_ms;
  }
  return total;
}

double CalculateDiskOpsPerSec(const DiskStats& prev, const DiskStats& curr,
                              double interval_sec) {
  if (interval_sec <= 0) return 0.0;
  unsigned long diff = (curr.reads + curr.writes) - (prev.reads + prev.writes);
  return static_cast<double>(diff) / interval_sec;
}

double CalculateDiskThroughputMB(const DiskStats& prev, const DiskStats& curr,
                                 double interval_sec) {
  if (interval_sec <= 0) return 0.0;
  // sectors are 512 bytes
  unsigned long sectors_diff = (curr.read_sectors + curr.write_sectors) -
                               (prev.read_sectors + prev.write_sectors);
  double bytes = static_cast<double>(sectors_diff) * 512.0;
  return bytes / (1024.0 * 1024.0) / interval_sec;  // MB/s
}

double GetDiskVolumeGB() {
  struct statvfs buf;
  if (statvfs("/", &buf) != 0) return 0.0;
  double total_bytes =
      static_cast<double>(buf.f_blocks) * static_cast<double>(buf.f_frsize);
  return total_bytes / (1024.0 * 1024.0 * 1024.0);  // GB
}

double GetDiskUsagePercent() {
  struct statvfs buf;
  if (statvfs("/", &buf) != 0) return 0.0;
  double used = static_cast<double>(buf.f_blocks - buf.f_bfree);
  double total = static_cast<double>(buf.f_blocks);
  if (total == 0) return 0.0;
  return 100.0 * used / total;
}

class MemoryAgent : public BaseAgent {
 public:
  explicit MemoryAgent(const std::string& config_path)
      : BaseAgent(config_path, "memory_agent", AgentType::kMemory,
                  {"ram_total", "ram", "hard_volume", "hard_ops",
                   "hard_throughput", "hard_usage"}) {}

  std::vector<Metric> UpdateMetrics(std::stop_token stop_token) override {
    std::vector<Metric> metrics;
    if (stop_token.stop_requested()) return {};
    auto now = chr::steady_clock::now();
    double interval_sec = 0.0;
    if (last_time_.time_since_epoch().count() > 0) {
      interval_sec = std::chrono::duration<double>(now - last_time_).count();
    }
    last_time_ = now;

    // RAM
    auto meminfo = ParseMemInfo();
    auto total_it = meminfo.find("MemTotal");
    if (total_it != meminfo.end()) {
      metrics.push_back(
          {"ram_total", static_cast<double>(total_it->second) / 1024.0});  // MB
    }
    metrics.push_back({"ram", GetRamUsagePercent(meminfo)});

    // Disk volume
    metrics.push_back({"hard_volume", GetDiskVolumeGB()});
    metrics.push_back({"hard_usage", GetDiskUsagePercent()});

    // Disk I/O (delta-based)
    auto curr_disk = ReadTotalDiskStats();
    if (prev_disk_.reads + prev_disk_.writes > 0 && interval_sec > 0) {
      metrics.push_back({"hard_ops", CalculateDiskOpsPerSec(
                                         prev_disk_, curr_disk, interval_sec)});
      metrics.push_back(
          {"hard_throughput",
           CalculateDiskThroughputMB(prev_disk_, curr_disk, interval_sec)});
    }
    prev_disk_ = curr_disk;

    return metrics;
  }

 private:
  DiskStats prev_disk_;
  TimePoint last_time_;
};

}  // namespace s21

extern "C" s21::IAgent* create_agent(const char* config_path) {
  return new s21::MemoryAgent(config_path);
}
