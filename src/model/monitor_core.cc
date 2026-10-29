#include "monitor_core.h"

#include <algorithm>
#include <ranges>

namespace s21 {

MonitorCore::MonitorCore(Path agents_dir, Path logs_dir,
                         int update_interval_seconds)
    : agent_manager_(std::move(agents_dir)),
      logger_(std::move(logs_dir)),
      default_interval_(update_interval_seconds) {}

MonitorCore::~MonitorCore() { Stop(); }

void MonitorCore::Start() {
  agent_manager_.StartScanning(ScanAgentsInterval);
  worker_thread_ = std::jthread(&MonitorCore::WorkLoop, this);
}

void MonitorCore::Stop() {
  worker_thread_.request_stop();
  agent_manager_.StopScanning();
}

std::vector<std::shared_ptr<IAgent>> MonitorCore::GetAgents() const {
  return agent_manager_.GetAgents();
}

std::vector<std::string> MonitorCore::GetLastLogLines(int count) const {
  return logger_.GetLastLines(count);
}

void MonitorCore::UpdateAgentConfig(const std::string& agent_name,
                                    const AgentConfig& config) {
  auto agents = agent_manager_.GetAgents();
  auto it = std::ranges::find(agents, agent_name, &IAgent::GetName);
  if (it != agents.end()) {
    (*it)->SetConfig(config);
    ConfigParser::Save(config.config_path, config);
  }
}

void MonitorCore::DisableAgent(const std::string& agent_name) {
  agent_manager_.DisableAgent(agent_name);
}

void MonitorCore::SetTelegramCredentials(const std::string& token,
                                         const std::string& chat_id,
                                         const std::string& proxy) {
  alert_notifier_.SetTelegramCredentials(token, chat_id, proxy);
}

void MonitorCore::SetEmailCredentials(const std::string& smtp_server,
                                      int smtp_port,
                                      const std::string& username,
                                      const std::string& password,
                                      const std::string& from_address) {
  alert_notifier_.SetEmailCredentials(smtp_server, smtp_port, username,
                                      password, from_address);
}

void MonitorCore::SetEmailRecipient(const std::string& to_address) {
  alert_notifier_.SetEmailRecipient(to_address);
}

void MonitorCore::SetEmailEnabled(bool enabled) {
  alert_notifier_.SetEmailEnabled(enabled);
}

void MonitorCore::SetTelegramCurlTimeout(long timeout_sec) {
  alert_notifier_.SetTelegramCurlTimeout(timeout_sec);
}

void MonitorCore::SetEmailCurlTimeout(long timeout_sec) {
  alert_notifier_.SetEmailCurlTimeout(timeout_sec);
}

MonitorCore::IterationResult MonitorCore::ProcessActiveAgents(
    auto&& active_agents, TimePoint now, std::stop_token stop_token) {
  IterationResult result{.seconds_until_next_wake =
                             std::numeric_limits<int>::max()};

  for (const auto& agent : active_agents) {
    if (stop_token.stop_requested()) [[unlikely]]
      break;

    PollSingleAgent(agent, now, result, stop_token);
  }

  if (result.seconds_until_next_wake == std::numeric_limits<int>::max()) {
    result.seconds_until_next_wake = default_interval_;
  }

  return result;
}

void MonitorCore::PollSingleAgent(const std::shared_ptr<IAgent>& agent,
                                  TimePoint now, IterationResult& result,
                                  std::stop_token stop_token) {
  int agent_interval = agent->GetConfig().update_interval_seconds;
  auto last_update = agent->GetLastUpdateTime();

  long long seconds_elapsed = 0;
  if (last_update != TimePoint::min()) {
    seconds_elapsed =
        chr::duration_cast<chr::seconds>(now - last_update).count();
  } else {
    seconds_elapsed = agent_interval;
  }

  if (seconds_elapsed >= agent_interval) {
    try {
      auto metrics = agent->UpdateMetrics(stop_token);
      agent->SetLastUpdateTime(now);

      if (!metrics.empty()) {
        auto config = agent->GetConfig();
        auto alerts = CriticalChecker::Check(metrics, config.thresholds);

        std::ranges::move(metrics, std::back_inserter(result.metrics));
        std::ranges::move(alerts, std::back_inserter(result.alerts));
      }

      result.seconds_until_next_wake =
          std::min(result.seconds_until_next_wake, agent_interval);
      result.data_collected = true;
    } catch (...) {
    }
  } else {
    int seconds_remaining = agent_interval - static_cast<int>(seconds_elapsed);
    result.seconds_until_next_wake =
        std::min(result.seconds_until_next_wake, seconds_remaining);
  }
}

void MonitorCore::Wait(chr::seconds duration, std::stop_token stop_token) {
  std::unique_lock<std::mutex> lock(cv_m_);
  cv_.wait_for(lock, stop_token, duration,
               [&stop_token] { return stop_token.stop_requested(); });
}

void MonitorCore::WorkLoop(std::stop_token stop_token) {
  while (!stop_token.stop_requested()) {
    try {
      auto all_agents = agent_manager_.GetAgents();
      auto active_agents =
          all_agents | std::views::filter([](const auto& agent) {
            return agent && agent->IsActive();
          });

      if (std::ranges::empty(active_agents)) {
        Wait(chr::seconds(default_interval_), stop_token);
        continue;
      }

      auto now = chr::steady_clock::now();
      IterationResult result =
          ProcessActiveAgents(active_agents, now, stop_token);

      if (result.data_collected) {
        if (!result.metrics.empty()) logger_.LogMetrics(result.metrics);
        if (!result.alerts.empty())
          alert_notifier_.ProcessAlerts(result.alerts);
      }

      int final_sleep_time = std::max(1, result.seconds_until_next_wake);
      Wait(chr::seconds(final_sleep_time), stop_token);

    } catch (...) {
      Wait(chr::seconds(default_interval_), stop_token);
    }
  }
}

}  // namespace s21
