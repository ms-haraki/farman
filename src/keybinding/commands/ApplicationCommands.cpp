#include "ApplicationCommands.h"
#include "ui/MainWindow.h"
#include <QApplication>
#include <QDebug>

namespace Farman {

void AppQuitCommand::execute() {
  qDebug() << "AppQuitCommand::execute()";
  QApplication::quit();
}

void ViewFileCommand::execute() {
  // Stub implementation - will be connected to MainWindow later
  qDebug() << "ViewFileCommand::execute() - stub";
}

} // namespace Farman
