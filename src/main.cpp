#include <QApplication>
#include "ui/MainWindow.h"

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);

  app.setOrganizationName("Farman");
  app.setApplicationName("farman");

  Farman::MainWindow window;
  window.show();

  return app.exec();
}
