#include "main_window.h"

#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QSettings>
#include <QVBoxLayout>

#include "view/agent_config_dialog.h"

namespace s21 {

MainWindow::MainWindow(MonitorController* controller, QWidget* parent)
    : QMainWindow(parent), controller_(controller) {
  SetupUi();
  connect(controller_, &MonitorController::AgentsChanged, this,
          &MainWindow::OnAgentsChanged);
  connect(controller_, &MonitorController::LogUpdated, this,
          &MainWindow::OnLogUpdated);
  OnAgentsChanged();
  OnLogUpdated();
  SetControllerSettings(LoadSettings());
}

MainWindow::~MainWindow() = default;

void MainWindow::SetupUi() {
  auto* central = new QWidget(this);
  auto* main_layout = new QVBoxLayout(central);

  auto* agents_group = new QGroupBox("Agents", this);
  auto* agents_layout = new QVBoxLayout(agents_group);
  agents_table_ = new QTableWidget(0, 4, this);
  agents_table_->setHorizontalHeaderLabels(
      {"Name", "Type", "Uptime", "Active"});
  agents_table_->horizontalHeader()->setStretchLastSection(true);
  agents_table_->setSelectionBehavior(QAbstractItemView::SelectRows);
  agents_layout->addWidget(agents_table_);

  auto* btn_layout = new QHBoxLayout();
  configure_btn_ = new QPushButton("Configure", this);
  disable_btn_ = new QPushButton("Enable/Disable", this);
  settings_btn_ = new QPushButton("Settings", this);
  btn_layout->addWidget(configure_btn_);
  btn_layout->addWidget(disable_btn_);
  btn_layout->addWidget(settings_btn_);
  agents_layout->addLayout(btn_layout);

  main_layout->addWidget(agents_group);

  auto* log_group = new QGroupBox("Logs", this);
  auto* log_layout = new QVBoxLayout(log_group);
  log_view_ = new QPlainTextEdit(this);
  log_view_->setReadOnly(true);
  log_view_->ensureCursorVisible();
  log_view_->setMaximumBlockCount(21);
  log_layout->addWidget(log_view_);
  main_layout->addWidget(log_group);

  setCentralWidget(central);
  resize(800, 600);

  connect(configure_btn_, &QPushButton::clicked, this,
          &MainWindow::OnConfigureAgent);
  connect(disable_btn_, &QPushButton::clicked, this,
          &MainWindow::OnDisableAgent);
  connect(settings_btn_, &QPushButton::clicked, this,
          &MainWindow::OnOpenSettings);
}

QString MainWindow::GetSelectedAgentName() const {
  int row = agents_table_->currentRow();
  if (row < 0 || row >= agents_table_->rowCount()) return QString();
  auto* item = agents_table_->item(row, 0);
  return item ? item->text() : QString();
}

void MainWindow::RestoreSelection(const QString& agent_name) {
  if (agent_name.isEmpty()) return;
  for (int row = 0; row < agents_table_->rowCount(); ++row) {
    auto* item = agents_table_->item(row, 0);
    if (item && item->text() == agent_name) {
      agents_table_->setCurrentItem(item);
      return;
    }
  }
}

void MainWindow::RefreshAgentsTable() {
  QString selected_name = GetSelectedAgentName();
  agents_table_->setRowCount(0);
  auto agents = controller_->GetAgents();
  for (const auto& agent : agents) {
    int row = agents_table_->rowCount();
    agents_table_->insertRow(row);
    agents_table_->setItem(
        row, 0, new QTableWidgetItem(QString::fromStdString(agent->GetName())));
    agents_table_->setItem(row, 1,
                           new QTableWidgetItem(QString::fromUtf8(
                               AgentTypeToString(agent->GetType()))));

    auto uptime =
        std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) -
        agent->GetStartTime();
    agents_table_->setItem(row, 2,
                           new QTableWidgetItem(QString::number(uptime) + "s"));
    agents_table_->setItem(
        row, 3, new QTableWidgetItem(agent->IsActive() ? "Yes" : "No"));
  }
  RestoreSelection(selected_name);
}

void MainWindow::OnAgentsChanged() { RefreshAgentsTable(); }

void MainWindow::OnLogUpdated() {
  auto lines = controller_->GetLastLogLines(20);
  QString text;
  for (const auto& line : lines) {
    text += QString::fromStdString(line) + "\n";
  }
  QTextCursor cursor = log_view_->textCursor();
  cursor.movePosition(QTextCursor::End);
  log_view_->setPlainText(text);
  log_view_->setTextCursor(cursor);
}

void MainWindow::OnConfigureAgent() {
  int row = agents_table_->currentRow();
  if (row < 0) return;
  auto agents = controller_->GetAgents();
  if (row >= static_cast<int>(agents.size())) return;

  AgentConfigDialog dialog(agents[row]->GetConfig(), this);
  if (dialog.exec() == QDialog::Accepted) {
    controller_->UpdateAgentConfig(agents[row]->GetName(), dialog.GetConfig());
  }
}

void MainWindow::OnDisableAgent() {
  int row = agents_table_->currentRow();
  if (row < 0) return;
  auto agents = controller_->GetAgents();
  if (row >= static_cast<int>(agents.size())) return;
  controller_->DisableAgent(agents[row]->GetName());
}

void MainWindow::OnOpenSettings() {
  NotificationSettings current_settings = LoadSettings();

  SettingsDialog dialog(current_settings, this);
  if (dialog.exec() == QDialog::Accepted) {
    NotificationSettings new_settings = dialog.GetSettings();
    SaveSettings(new_settings);
    SetControllerSettings(new_settings);
  }
}

void MainWindow::SetControllerSettings(const NotificationSettings& settings) {
  controller_->SetTelegramCredentials(
      settings.telegram_token, settings.telegram_chat_id, settings.proxy);
  controller_->SetEmailCredentials(
      settings.smtp_server, settings.smtp_port, settings.email_username,
      settings.email_password, settings.email_from);
  controller_->SetEmailRecipient(settings.email_to);
  controller_->SetEmailEnabled(settings.email_enabled);
}

NotificationSettings MainWindow::LoadSettings() {
  QSettings settings("s21", "MyMonitor");
  NotificationSettings s;

  s.telegram_token =
      settings.value("telegram/token", "").toString().toStdString();
  s.telegram_chat_id =
      settings.value("telegram/chat_id", "").toString().toStdString();
  s.proxy = settings.value("proxy", "").toString().toStdString();

  s.email_enabled = settings.value("email/enabled", false).toBool();
  s.smtp_server =
      settings.value("email/smtp_server", "").toString().toStdString();
  s.smtp_port = settings.value("email/smtp_port", 587).toInt();
  s.email_username =
      settings.value("email/username", "").toString().toStdString();
  s.email_password =
      settings.value("email/password", "").toString().toStdString();
  s.email_from = settings.value("email/from", "").toString().toStdString();
  s.email_to = settings.value("email/to", "").toString().toStdString();

  return s;
}

void MainWindow::SaveSettings(const NotificationSettings& s) {
  QSettings settings("s21", "MyMonitor");

  settings.setValue("telegram/token", QString::fromStdString(s.telegram_token));
  settings.setValue("telegram/chat_id",
                    QString::fromStdString(s.telegram_chat_id));
  settings.setValue("proxy", QString::fromStdString(s.proxy));

  settings.setValue("email/enabled", s.email_enabled);
  settings.setValue("email/smtp_server", QString::fromStdString(s.smtp_server));
  settings.setValue("email/smtp_port", s.smtp_port);
  settings.setValue("email/username", QString::fromStdString(s.email_username));
  settings.setValue("email/password", QString::fromStdString(s.email_password));
  settings.setValue("email/from", QString::fromStdString(s.email_from));
  settings.setValue("email/to", QString::fromStdString(s.email_to));
}

}  // namespace s21
