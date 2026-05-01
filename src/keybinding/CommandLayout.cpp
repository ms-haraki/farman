#include "CommandLayout.h"
#include "CommandRegistry.h"
#include <QCoreApplication>

namespace Farman {

namespace {

// カテゴリの表示順と表示名 (tr() 経由)。ここを編集すれば全表示先で順序が揃う。
QList<CommandCategoryGroup> categoryTemplate() {
  return {
    { QStringLiteral("navigation"), QCoreApplication::translate("CommandLayout", "Navigation"),     {} },
    { QStringLiteral("selection"),  QCoreApplication::translate("CommandLayout", "Selection"),      {} },
    { QStringLiteral("file"),       QCoreApplication::translate("CommandLayout", "File operations"),{} },
    { QStringLiteral("pane"),       QCoreApplication::translate("CommandLayout", "Pane"),           {} },
    { QStringLiteral("view"),       QCoreApplication::translate("CommandLayout", "View / Viewer"),  {} },
    { QStringLiteral("bookmark"),   QCoreApplication::translate("CommandLayout", "Bookmark"),       {} },
    { QStringLiteral("history"),    QCoreApplication::translate("CommandLayout", "History"),        {} },
    { QStringLiteral("app"),        QCoreApplication::translate("CommandLayout", "Application"),    {} },
    { QStringLiteral("help"),       QCoreApplication::translate("CommandLayout", "Help"),           {} },
    { QStringLiteral("general"),    QCoreApplication::translate("CommandLayout", "Other"),          {} },
  };
}

} // namespace

QList<CommandCategoryGroup> commandsGroupedByCategory(
  const CommandRegistry& registry)
{
  QList<CommandCategoryGroup> groups = categoryTemplate();
  // ID → group インデックスのマップ
  QMap<QString, int> idx;
  for (int i = 0; i < groups.size(); ++i) idx.insert(groups[i].id, i);

  // CommandRegistry::allCommands() は登録順で返るのでそのまま流し込む。
  for (ICommand* cmd : registry.allCommands()) {
    if (!cmd) continue;
    const int i = idx.value(cmd->category(),
                            idx.value(QStringLiteral("general"), -1));
    if (i < 0) continue;  // 念のため
    groups[i].commands.append(cmd);
  }

  // 空のカテゴリは結果から除く
  QList<CommandCategoryGroup> result;
  for (auto& g : groups) {
    if (!g.commands.isEmpty()) result.append(std::move(g));
  }
  return result;
}

} // namespace Farman
