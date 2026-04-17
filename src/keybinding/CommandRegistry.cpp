#include "CommandRegistry.h"
#include <QDebug>

namespace Farman {

CommandRegistry& CommandRegistry::instance() {
  static CommandRegistry instance;
  return instance;
}

CommandRegistry::CommandRegistry(QObject* parent) : QObject(parent) {
}

void CommandRegistry::registerCommand(std::shared_ptr<ICommand> cmd) {
  if (!cmd) {
    qWarning() << "CommandRegistry::registerCommand: null command";
    return;
  }

  const QString id = cmd->id();
  if (id.isEmpty()) {
    qWarning() << "CommandRegistry::registerCommand: command with empty id";
    return;
  }

  if (m_commands.contains(id)) {
    qWarning() << "CommandRegistry::registerCommand: command already registered:" << id;
    return;
  }

  m_commands.insert(id, cmd);
  qDebug() << "CommandRegistry: registered command" << id << ":" << cmd->label();
}

bool CommandRegistry::execute(const QString& commandId) {
  auto it = m_commands.find(commandId);
  if (it == m_commands.end()) {
    qWarning() << "CommandRegistry::execute: unknown command:" << commandId;
    return false;
  }

  ICommand* cmd = it.value().get();
  if (!cmd->isEnabled()) {
    qDebug() << "CommandRegistry::execute: command disabled:" << commandId;
    return false;
  }

  try {
    cmd->execute();
    return true;
  } catch (const std::exception& e) {
    qWarning() << "CommandRegistry::execute: exception in command" << commandId << ":" << e.what();
    return false;
  } catch (...) {
    qWarning() << "CommandRegistry::execute: unknown exception in command" << commandId;
    return false;
  }
}

ICommand* CommandRegistry::command(const QString& commandId) const {
  auto it = m_commands.find(commandId);
  if (it == m_commands.end()) {
    return nullptr;
  }
  return it.value().get();
}

QList<ICommand*> CommandRegistry::allCommands() const {
  QList<ICommand*> result;
  result.reserve(m_commands.size());

  for (const auto& cmd : m_commands) {
    result.append(cmd.get());
  }

  return result;
}

QList<ICommand*> CommandRegistry::commandsByCategory(const QString& category) const {
  QList<ICommand*> result;

  for (const auto& cmd : m_commands) {
    if (cmd->category() == category) {
      result.append(cmd.get());
    }
  }

  return result;
}

} // namespace Farman
