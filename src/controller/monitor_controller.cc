#include "monitor_controller.h"

#include <QDateTime>
#include <chrono>

namespace s21 {

MonitorController::MonitorController(MonitorCore* core, QObject* parent)
    : QObject(parent), core_(core) {
  poll_timer_ = new QTimer(this);
  poll_timer_->setTimerType(Qt::CoarseTimer);
  connect(poll_timer_, &QTimer::timeout, this, &MonitorController::OnPoll,
          Qt::QueuedConnection);
}

MonitorController::~MonitorController() = default;

void MonitorController::Start() {
  core_->Start();
  poll_timer_->start(500);
  emit LogUpdated();
  emit AgentsChanged();
}

void MonitorController::Stop() {
  poll_timer_->stop();
  core_->Stop();
}

std::vector<std::shared_ptr<IAgent>> MonitorController::GetAgents() const {
  return core_->GetAgents();
}

std::vector<std::string> MonitorController::GetLastLogLines(int count) const {
  return core_->GetLastLogLines(count);
}

void MonitorController::DisableAgent(const std::string& name) {
  core_->DisableAgent(name);
  emit AgentsChanged();
}

void MonitorController::UpdateAgentConfig(const std::string& name,
                                          const AgentConfig& config) {
  core_->UpdateAgentConfig(name, config);
  emit AgentsChanged();
}

void MonitorController::SetTelegramCredentials(const std::string& token,
                                               const std::string& chat_id,
                                               const std::string& proxy) {
  core_->SetTelegramCredentials(token, chat_id, proxy);
}

void MonitorController::SetEmailCredentials(const std::string& smtp_server,
                                            int smtp_port,
                                            const std::string& username,
                                            const std::string& password,
                                            const std::string& from_address) {
  core_->SetEmailCredentials(smtp_server, smtp_port, username, password,
                             from_address);
}

void MonitorController::SetEmailRecipient(const std::string& to_address) {
  core_->SetEmailRecipient(to_address);
}

void MonitorController::SetEmailEnabled(bool enabled) {
  core_->SetEmailEnabled(enabled);
}

void MonitorController::SetTelegramCurlTimeout(long timeout_sec) {
  core_->SetTelegramCurlTimeout(timeout_sec);
}

void MonitorController::SetEmailCurlTimeout(long timeout_sec) {
  core_->SetEmailCurlTimeout(timeout_sec);
}

void MonitorController::OnPoll() {
  emit LogUpdated();
  emit AgentsChanged();
}

}  // namespace s21
