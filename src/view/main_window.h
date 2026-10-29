#pragma once
#include <QMainWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTableWidget>
#include <memory>

#include "controller/monitor_controller.h"
#include "view/settings_dialog.h"

namespace s21 {

class MainWindow : public QMainWindow {
  Q_OBJECT

 public:
  explicit MainWindow(MonitorController* controller, QWidget* parent = nullptr);
  ~MainWindow();

 private slots:
  void OnAgentsChanged();
  void OnLogUpdated();
  void OnConfigureAgent();
  void OnDisableAgent();
  void OnOpenSettings();

 private:
  void SetupUi();
  void RefreshAgentsTable();
  void RestoreSelection(const QString& agent_name);
  void SaveSettings(const NotificationSettings& s);
  void SetControllerSettings(const NotificationSettings& settings);
  NotificationSettings LoadSettings();
  [[nodiscard]] QString GetSelectedAgentName() const;

  MonitorController* controller_;
  QTableWidget* agents_table_;
  QPlainTextEdit* log_view_;
  QPushButton* configure_btn_;
  QPushButton* disable_btn_;
  QPushButton* settings_btn_;
};

}  // namespace s21
