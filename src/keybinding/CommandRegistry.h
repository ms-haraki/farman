#pragma once

#include "ICommand.h"
#include <QObject>
#include <QMap>
#include <memory>

namespace Farman {

class CommandRegistry : public QObject {
  Q_OBJECT

public:
  static CommandRegistry& instance();

  void registerCommand(std::shared_ptr<ICommand> cmd);

  // コマンドIDから実行
  bool execute(const QString& commandId);

  // コマンドIDからコマンドを取得
  ICommand* command(const QString& commandId) const;

  // 全コマンド一覧（設定画面のキーバインド一覧に使用）
  QList<ICommand*> allCommands() const;
  QList<ICommand*> commandsByCategory(const QString& category) const;

private:
  explicit CommandRegistry(QObject* parent = nullptr);
  QMap<QString, std::shared_ptr<ICommand>> m_commands;
};

} // namespace Farman
