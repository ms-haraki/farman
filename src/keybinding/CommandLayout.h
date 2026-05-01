#pragma once

#include "ICommand.h"
#include <QList>
#include <QString>

namespace Farman {

class CommandRegistry;

// カテゴリ別の表示用情報。`commandsGroupedByCategory()` が返す。
struct CommandCategoryGroup {
  QString          id;        // カテゴリ ID ("navigation" 等)
  QString          display;   // 表示名 (`tr()` 適用済み)
  QList<ICommand*> commands;  // そのカテゴリに属するコマンド (登録順)
};

// CommandRegistry::allCommands() を「事前定義したカテゴリ順」で
// グループ化して返す。ショートカット一覧 / Keybindings タブ等、複数の
// UI で表示順を統一するための共通ヘルパ。
//
// - カテゴリ順は: navigation → selection → file → pane → view →
//   bookmark → history → app → help → general (Other)。
// - 各カテゴリ内部の順序は CommandRegistry の登録順 (= MainWindow::
//   registerCommands での記述順) を保持する。
// - 既知でないカテゴリのコマンドは "general" (= Other) に集約される。
// - 空のカテゴリは結果に含めない。
QList<CommandCategoryGroup> commandsGroupedByCategory(
  const CommandRegistry& registry);

} // namespace Farman
