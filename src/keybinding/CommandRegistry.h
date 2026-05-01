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

  // 全コマンド一覧（設定画面のキーバインド一覧に使用）。
  // 戻り値の並びは「登録順」(= MainWindow::registerCommands での記述順)。
  // ショートカット一覧 / Keybindings タブのどちらも、ここから順に拾うことで
  // 表示順を統一できる。
  QList<ICommand*> allCommands() const;
  QList<ICommand*> commandsByCategory(const QString& category) const;

private:
  explicit CommandRegistry(QObject* parent = nullptr);
  QMap<QString, std::shared_ptr<ICommand>> m_commands;
  // 登録順を保持する (m_commands は QMap でキー順なので別途保持)。
  QList<QString>                            m_orderedIds;
};

} // namespace Farman
