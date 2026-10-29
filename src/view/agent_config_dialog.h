#pragma once
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QLineEdit>
#include <QListWidget>
#include <QSpinBox>
#include <QTableWidget>

#include "model/agent_config.h"

namespace s21 {

class AgentConfigDialog : public QDialog {
  Q_OBJECT

 public:
  explicit AgentConfigDialog(const AgentConfig& config,
                             QWidget* parent = nullptr);
  [[nodiscard]] AgentConfig GetConfig() const;

 private:
  AgentConfig config_;
  QLineEdit* url_edit_;
  QLineEdit* name_edit_;
  QSpinBox* interval_spin_;
  QListWidget* metrics_list_;
  QTableWidget* thresholds_table_;
};

}  // namespace s21
