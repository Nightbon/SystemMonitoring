#pragma once
#include <memory>
#include <stop_token>
#include <string>
#include <thread>
#include <vector>

#include "agent_manager.h"
#include "alert_notifier.h"
#include "config_parser.h"
#include "critical_checker.h"
#include "logger.h"

namespace s21 {

class MonitorCore {
 public:
  MonitorCore(Path agents_dir, Path logs_dir, int update_interval_seconds);
  ~MonitorCore();

  void Start();
  void Stop();

  [[nodiscard]] std::vector<std::shared_ptr<IAgent>> GetAgents() const;
  [[nodiscard]] std::vector<std::string> GetLastLogLines(int count) const;

  void UpdateAgentConfig(const std::string& agent_name,
                         const AgentConfig& config);
  void DisableAgent(const std::string& agent_name);
  void SetTelegramCredentials(const std::string& token,
                              const std::string& chat_id,
                              const std::string& proxy);
  void SetEmailCredentials(const std::string& smtp_server, int smtp_port,
                           const std::string& username,
                           const std::string& password,
                           const std::string& from_address);
  void SetEmailRecipient(const std::string& to_address);
  void SetEmailEnabled(bool enabled);
  void SetTelegramCurlTimeout(long timeout_sec);
  void SetEmailCurlTimeout(long timeout_sec);

 private:
  AgentManager agent_manager_;
  Logger logger_;
  AlertNotifier alert_notifier_;
  std::mutex cv_m_;
  std::condition_variable_any cv_;
  int default_interval_;

  struct IterationResult {
    std::vector<Metric> metrics;
    std::vector<std::string> alerts;
    int seconds_until_next_wake = 0;
    bool data_collected = false;
  };

  IterationResult ProcessActiveAgents(auto&& active_agents, TimePoint now,
                                      std::stop_token stop_token);

  void PollSingleAgent(const std::shared_ptr<IAgent>& agent, TimePoint now,
                       IterationResult& result, std::stop_token stop_token);

  void Wait(chr::seconds duration, std::stop_token stop_token);
  void WorkLoop(std::stop_token stop_token);
  std::jthread worker_thread_;
};

}  // namespace s21
