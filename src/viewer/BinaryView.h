#pragma once

#include <QPlainTextEdit>

namespace Farman {

// 16 進ダンプ表示用の読み取り専用ビュー。
// 1 行あたり: "AAAAAAAA  HH HH HH HH HH HH HH HH  HH HH HH HH HH HH HH HH  cccccccccccccccc"
// アドレスは 8 桁 16 進、16 byte 単位、ASCII (0x20–0x7E) のみ可視化、それ以外は '.'。
class BinaryView : public QPlainTextEdit {
  Q_OBJECT

public:
  explicit BinaryView(QWidget* parent = nullptr);

  // 失敗時 (open エラー) は false。
  bool loadFile(const QString& filePath);

private:
  // 大きすぎるファイルの読み込みを抑制するためのソフト上限 (bytes)。
  // 超過分は読み飛ばし、末尾に注記行を付与する。
  static constexpr qint64 kMaxBytes = 8 * 1024 * 1024;
};

} // namespace Farman
