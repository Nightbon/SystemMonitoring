#include "agent_config_dialog.h"

#include <QFormLayout>
#include <QLabel>
#include <QVBoxLayout>

namespace s21 {

AgentConfigDialog::AgentConfigDialog(const AgentConfig& config, QWidget* parent)
    : QDialog(parent), config_(config) {
  setWindowTitle("Agent Configuration");
  auto* layout = new QFormLayout(this);

  name_edit_ = new QLineEdit(QString::fromStdString(config_.name), this);
  name_edit_->setReadOnly(true);
  layout->addRow("Name:", name_edit_);

  interval_spin_ = new QSpinBox(this);
  interval_spin_->setRange(1, 3600);
  interval_spin_->setValue(config_.update_interval_seconds);
  layout->addRow("Update Interval (s):", interval_spin_);

  metrics_list_ = new QListWidget(this);
  for (const auto& m : config_.metrics) {
    metrics_list_->addItem(QString::fromStdString(m));
  }
  metrics_list_->setSelectionMode(QAbstractItemView::NoSelection);
  layout->addRow("Metrics:", metrics_list_);

  thresholds_table_ = new QTableWidget(this);
  thresholds_table_->setColumnCount(3);
  thresholds_table_->setHorizontalHeaderLabels({"Metric", "Operator", "Value"});
  thresholds_table_->horizontalHeader()->setSectionResizeMode(
      QHeaderView::Stretch);
  thresholds_table_->setRowCount(static_cast<int>(config_.thresholds.size()));

  const QStringList operators = {">", "<", ">=", "<=", "==", "!="};

  for (int i = 0; i < thresholds_table_->rowCount(); ++i) {
    const auto& th = config_.thresholds[i];
    QString name;
    if (th.metric_name == "ping_url") {
      name = QString::fromStdString(config_.ping_url);
    } else {
      name = QString::fromStdString(th.metric_name);
    }
    auto* metric_item = new QTableWidgetItem(name);
    metric_item->setFlags(metric_item->flags() & ~Qt::ItemIsEditable);
    thresholds_table_->setItem(i, 0, metric_item);

    auto* op_combo = new QComboBox(this);
    op_combo->addItems(operators);
    int op_index = operators.indexOf(QString::fromStdString(th.op));
    if (op_index != -1) op_combo->setCurrentIndex(op_index);
    thresholds_table_->setCellWidget(i, 1, op_combo);

    auto* value_spin = new QDoubleSpinBox(this);
    value_spin->setRange(-1000000.0, 1000000.0);
    value_spin->setDecimals(2);
    value_spin->setValue(th.value);
    thresholds_table_->setCellWidget(i, 2, value_spin);
  }
  layout->addRow("Critical Thresholds:", thresholds_table_);
  // -------------------------------------

  url_edit_ = new QLineEdit(QString::fromStdString(config_.ping_url), this);
  layout->addRow("Ping URL:", url_edit_);
  url_edit_->setVisible(config_.type == AgentType::kNetwork);

  auto* buttons = new QDialogButtonBox(
      QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
  layout->addWidget(buttons);
  connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

AgentConfig AgentConfigDialog::GetConfig() const {
  AgentConfig result = config_;
  result.update_interval_seconds = interval_spin_->value();
  result.ping_url = url_edit_->text().toStdString();

  // Сохраняем измененные пороги из таблицы назад в структуру
  for (int i = 0; i < thresholds_table_->rowCount(); ++i) {
    auto* op_combo =
        qobject_cast<QComboBox*>(thresholds_table_->cellWidget(i, 1));
    auto* value_spin =
        qobject_cast<QDoubleSpinBox*>(thresholds_table_->cellWidget(i, 2));

    if (op_combo && value_spin) {
      result.thresholds[i].op = op_combo->currentText().toStdString();
      result.thresholds[i].value = value_spin->value();
    }
  }

  return result;
}

}  // namespace s21
