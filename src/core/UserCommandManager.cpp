#include "UserCommandManager.h"
#include "UserCommandRunner.h"
#include "PlaceholderExpander.h"
#include "Logger.h"
#include "settings/Settings.h"
#include "keybinding/CommandRegistry.h"
#include "keybinding/ICommand.h"

#include <QDebug>

namespace Farman {

UserCommandManager& UserCommandManager::instance() {
  static UserCommandManager s;
  return s;
}

UserCommandManager::UserCommandManager(QObject* parent) : QObject(parent) {
}

void UserCommandManager::setContextProvider(ContextProvider provider) {
  m_contextProvider = std::move(provider);
}

void UserCommandManager::sync() {
  clearRegistered();

  m_commands = Settings::instance().userCommands();
  for (const UserCommand& cmd : m_commands) {
    registerOne(cmd);
  }

  qDebug() << "UserCommandManager: synced" << m_commands.size() << "user commands";
  emit userCommandsChanged();
}

void UserCommandManager::clearRegistered() {
  // 既に登録されている user.cmd.* を全部落とす。スナップショット (m_commands)
  // を信用するのではなく、CommandRegistry 側に問い合わせて拾うことで、過去の
  // sync で登録された ID も確実に解除できる。
  auto& registry = CommandRegistry::instance();
  const QList<ICommand*> all = registry.allCommands();
  for (ICommand* c : all) {
    if (c && c->id().startsWith(QStringLiteral("user.cmd."))) {
      registry.unregisterCommand(c->id());
    }
  }
}

void UserCommandManager::registerOne(const UserCommand& cmd) {
  if (cmd.id.isEmpty()) {
    qWarning() << "UserCommandManager: skipping command with empty id";
    return;
  }

  const QString commandId = cmd.id;
  // ラムダはクロージャに id だけをキャプチャし、実体は instance().run(id) 経由で
  // 引き直す。これにより Settings 変更で UserCommand 内容が変わっても、
  // CommandRegistry に古いコピーが残らない (sync が再登録するので OK だが、
  // 念のため二重に安全)。
  auto fn = [commandId]() {
    QString err;
    UserCommandManager::instance().run(commandId, &err);
    // run() 側で Logger に流れているのでここでの追加処理は不要。
    Q_UNUSED(err);
  };

  // ICommand のカテゴリは "tools" (CommandLayout 側で「Tools」グループに入る想定)
  CommandRegistry::instance().registerCommand(
    std::make_shared<LambdaCommand>(
      cmd.id,
      cmd.name,
      fn,
      QStringLiteral("tools"),
      QObject::tr("External command: %1").arg(cmd.program)
    )
  );
}

PaneContext UserCommandManager::currentContext() const {
  if (m_contextProvider) {
    return m_contextProvider();
  }
  return PaneContext{};
}

bool UserCommandManager::run(const QString& commandId, QString* errorOut) {
  for (const UserCommand& c : m_commands) {
    if (c.id == commandId) {
      const PaneContext ctx = currentContext();
      return runUserCommand(c, ctx, errorOut);
    }
  }
  const QString msg = QObject::tr("Unknown external command: %1").arg(commandId);
  Logger::instance().warn(msg);
  if (errorOut) *errorOut = msg;
  return false;
}

bool UserCommandManager::runTransient(const UserCommand& cmd, QString* errorOut) {
  const PaneContext ctx = currentContext();
  return runUserCommand(cmd, ctx, errorOut);
}

} // namespace Farman
