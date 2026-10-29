#pragma once
#include <QObject>
#include <QTimer>
#include <memory>
#include <string>
#include <vector>

#include "model/agent_config.h"
#include "model/i_agent.h"
#include "model/monitor_core.h"

namespace s21 {

class MonitorController : public QObject {
  Q_OBJECT

 public:
  explicit MonitorController(MonitorCore* core, QObject* parent = nullptr);
  ~MonitorController();

  void Start();
  void Stop();

  std::vector<std::shared_ptr<IAgent>> GetAgents() const;
  std::vector<std::string> GetLastLogLines(int count) const;
  void DisableAgent(const std::string& name);
  void UpdateAgentConfig(const std::string& name, const AgentConfig& config);

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

 signals:
  void AgentsChanged();
  void LogUpdated();

 private slots:
  void OnPoll();

 private:
  MonitorCore* core_;
  QTimer* poll_timer_;
};

}  // namespace s21
