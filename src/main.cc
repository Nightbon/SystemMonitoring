#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QString>

#include "controller/monitor_controller.h"
#include "model/monitor_core.h"
#include "view/main_window.h"

int main(int argc, char* argv[]) {
  QApplication app(argc, argv);

  QString app_dir = QCoreApplication::applicationDirPath();
  QString agents_dir = app_dir + "/agents";
  QString logs_dir = app_dir + "/logs";

  s21::MonitorCore core(agents_dir.toStdString(), logs_dir.toStdString(), 5);
  s21::MonitorController controller(&core);
  s21::MainWindow window(&controller);

  window.show();
  controller.Start();

  int result = app.exec();
  controller.Stop();
  return result;
}
