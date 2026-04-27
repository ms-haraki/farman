#pragma once

#include <QPlainTextEdit>

namespace Farman {

// 操作ログ表示用の読み取り専用テキストペイン。`Logger::entryAppended` を購読して
// 新規行を追記する。構築時に Logger の現バッファを流し込んで履歴を再現する。
class LogPane : public QPlainTextEdit {
  Q_OBJECT

public:
  explicit LogPane(QWidget* parent = nullptr);
};

} // namespace Farman
