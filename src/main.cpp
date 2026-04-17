#include <QApplication>
#include "ui/MainWindow.h"
#include "viewer/ViewerDispatcher.h"

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);

  app.setOrganizationName("Farman");
  app.setApplicationName("farman");

  // Initialize built-in viewers
  Farman::ViewerDispatcher::instance().registerBuiltins();

  Farman::MainWindow window;
  window.show();

  return app.exec();
}
