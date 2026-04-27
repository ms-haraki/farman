#include "LogPane.h"
#include "core/Logger.h"

#include <QFontDatabase>
#include <QScrollBar>

namespace Farman {

LogPane::LogPane(QWidget* parent)
  : QPlainTextEdit(parent)
{
  setReadOnly(true);
  setLineWrapMode(QPlainTextEdit::NoWrap);
  setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
  setMaximumBlockCount(2000);

  // 高さは Settings の logPaneHeight を FileManagerPanel 側で setFixedHeight する。
  // 最低限 1 行ぶんは確保しておく。
  setMinimumHeight(fontMetrics().lineSpacing());

  // 既存の履歴を流し込む
  for (const QString& line : Logger::instance().recent()) {
    appendPlainText(line);
  }

  connect(&Logger::instance(), &Logger::entryAppended, this,
          [this](const QString& formatted) {
    appendPlainText(formatted);
    if (auto* sb = verticalScrollBar()) sb->setValue(sb->maximum());
  });
}

} // namespace Farman
