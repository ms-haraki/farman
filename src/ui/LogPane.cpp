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

  // 4 行ぶん程度の高さに収める (1 操作 1 行を想定)
  const int lineHeight = fontMetrics().lineSpacing();
  setMinimumHeight(lineHeight * 2);
  setMaximumHeight(lineHeight * 5);

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
