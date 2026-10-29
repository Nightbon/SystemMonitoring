#pragma once
#include <condition_variable>
#include <memory>
#include <shared_mutex>
#include <stop_token>
#include <string>
#include <thread>
#include <vector>

#include "agent_loader.h"
#include "i_agent.h"

namespace s21 {

class AgentManager {
 public:
  explicit AgentManager(Path agents_dir);
  ~AgentManager();

  void StartScanning(int interval_seconds);
  void StopScanning();
  void ScanAndLoad(std::stop_token stop_token = {});

  [[nodiscard]] std::vector<std::shared_ptr<IAgent>> GetAgents() const;
  void DisableAgent(const std::string& agent_name);

 private:
  void ScanLoop(std::stop_token stop_token = {});
  Path agents_dir_;
  std::vector<LoadedAgent> agents_;
  mutable std::shared_mutex mutex_;
  int scan_interval_ = ScanAgentsInterval;
  AgentLoader loader_;
  std::mutex cv_m_;
  std::condition_variable_any cv_;

  [[nodiscard]] bool IsAlreadyLoaded(const Path& path) const;
  std::jthread scan_thread_;
};

}  // namespace s21
