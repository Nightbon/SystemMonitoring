#include <array>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "model/base_agent.h"

namespace s21 {

struct NetDevStats {
  unsigned long long rx_bytes = 0;
  unsigned long long tx_bytes = 0;
  unsigned long long rx_packets = 0;
  unsigned long long tx_packets = 0;
};

NetDevStats ParseNetDevLine(const std::string& line) {
  NetDevStats stats;
  std::istringstream iss(line);
  std::string iface;
  iss >> iface;
  // Remove trailing colon
  if (!iface.empty() && iface.back() == ':') iface.pop_back();

  unsigned long long rx_bytes, rx_packets, rx_errs, rx_drop, rx_fifo, rx_frame;
  unsigned long long rx_compressed, rx_multicast;
  unsigned long long tx_bytes, tx_packets, tx_errs, tx_drop, tx_fifo, tx_colls;
  unsigned long long tx_carrier, tx_compressed;

  iss >> rx_bytes >> rx_packets >> rx_errs >> rx_drop >> rx_fifo >> rx_frame >>
      rx_compressed >> rx_multicast >> tx_bytes >> tx_packets >> tx_errs >>
      tx_drop >> tx_fifo >> tx_colls >> tx_carrier >> tx_compressed;

  stats.rx_bytes = rx_bytes;
  stats.tx_bytes = tx_bytes;
  stats.rx_packets = rx_packets;
  stats.tx_packets = tx_packets;
  return stats;
}

NetDevStats ReadTotalNetDev() {
  NetDevStats total;
  std::ifstream file("/proc/net/dev");
  std::string line;
  // Skip first two header lines
  std::getline(file, line);
  std::getline(file, line);
  while (std::getline(file, line)) {
    auto stats = ParseNetDevLine(line);
    total.rx_bytes += stats.rx_bytes;
    total.tx_bytes += stats.tx_bytes;
    total.rx_packets += stats.rx_packets;
    total.tx_packets += stats.tx_packets;
  }
  return total;
}

double CalculateThroughputMB(const NetDevStats& prev, const NetDevStats& curr,
                             double interval_sec) {
  if (interval_sec <= 0) return 0.0;
  unsigned long long bytes_diff =
      (curr.rx_bytes + curr.tx_bytes) - (prev.rx_bytes + prev.tx_bytes);
  double mb = static_cast<double>(bytes_diff) / (1024.0 * 1024.0);
  return mb / interval_sec;
}

int PingHost(const std::string& host, int count = 1, int timeout_sec = 2) {
  std::string cmd = "ping -c " + std::to_string(count) + " -W " +
                    std::to_string(timeout_sec) + " " + host +
                    " >/dev/null 2>&1";
  int result = std::system(cmd.c_str());
  return WEXITSTATUS(result) == 0 ? 1 : 0;
}

class NetworkAgent : public BaseAgent {
 public:
  explicit NetworkAgent(const std::string& config_path)
      : BaseAgent(config_path, "network_agent", AgentType::kNetwork,
                  {"ping_url", "inet_throughput"}) {}

  std::vector<Metric> UpdateMetrics(std::stop_token stop_token) override {
    std::vector<Metric> metrics;
    if (stop_token.stop_requested()) return metrics;
    auto now = chr::steady_clock::now();
    double interval_sec = 0.0;
    if (last_time_.time_since_epoch().count() > 0) {
      interval_sec = std::chrono::duration<double>(now - last_time_).count();
    }
    last_time_ = now;

    const std::string& url = GetConfig().ping_url;
    metrics.push_back({url, PingHost(url)});

    if (stop_token.stop_requested()) return metrics;

    auto curr_net = ReadTotalNetDev();
    if (prev_net_.rx_bytes + prev_net_.tx_bytes > 0 && interval_sec > 0) {
      metrics.push_back(
          {"inet_throughput",
           CalculateThroughputMB(prev_net_, curr_net, interval_sec)});
    }
    prev_net_ = curr_net;

    return metrics;
  }

 private:
  NetDevStats prev_net_;
  TimePoint last_time_;
};

}  // namespace s21

extern "C" s21::IAgent* create_agent(const char* config_path) {
  return new s21::NetworkAgent(config_path);
}
