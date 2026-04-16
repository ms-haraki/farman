#include "CommandRegistry.h"

namespace Farman {

CommandRegistry& CommandRegistry::instance() {
  static CommandRegistry instance;
  return instance;
}

CommandRegistry::CommandRegistry(QObject* parent) : QObject(parent) {
}

void CommandRegistry::registerCommand(std::shared_ptr<ICommand> cmd) {
  // TODO: 実装
}

bool CommandRegistry::execute(const QString& commandId) {
  // TODO: 実装
  return false;
}

ICommand* CommandRegistry::command(const QString& commandId) const {
  // TODO: 実装
  return nullptr;
}

QList<ICommand*> CommandRegistry::allCommands() const {
  // TODO: 実装
  return {};
}

QList<ICommand*> CommandRegistry::commandsByCategory(const QString& category) const {
  // TODO: 実装
  return {};
}

} // namespace Farman
