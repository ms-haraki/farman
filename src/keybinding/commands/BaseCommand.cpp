#include "BaseCommand.h"

namespace Farman {

BaseCommand::BaseCommand(MainWindow* mainWindow)
  : m_mainWindow(mainWindow) {
}

bool BaseCommand::isEnabled() const {
  return m_mainWindow != nullptr;
}

} // namespace Farman
