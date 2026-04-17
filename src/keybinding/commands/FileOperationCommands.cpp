#include "FileOperationCommands.h"
#include "ui/MainWindow.h"
#include <QDebug>

namespace Farman {

void FileCopyCommand::execute() {
  // Stub implementation - will be connected to MainWindow later
  qDebug() << "FileCopyCommand::execute() - stub";
}

void FileMoveCommand::execute() {
  // Stub implementation - will be connected to MainWindow later
  qDebug() << "FileMoveCommand::execute() - stub";
}

void FileDeleteCommand::execute() {
  // Stub implementation - will be connected to MainWindow later
  qDebug() << "FileDeleteCommand::execute() - stub";
}

void FileMkdirCommand::execute() {
  // Stub implementation - will be connected to MainWindow later
  qDebug() << "FileMkdirCommand::execute() - stub";
}

} // namespace Farman
