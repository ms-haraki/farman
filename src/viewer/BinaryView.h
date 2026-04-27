#pragma once

#include "types.h"
#include <QByteArray>
#include <QString>
#include <QWidget>

class QComboBox;
class QPlainTextEdit;

namespace Farman {

class AddressHighlighter;

// 16 進ダンプ表示用の読み取り専用ビュー。
// 上部のツールバーで unit / endian / encoding をその場で変更できるが、
// 設定ファイルには保存しない (Settings 側のデフォルトは別途 Appearance タブで編集)。
// `Settings::settingsChanged` でグローバル設定が変わると、ローカルの上書きはリセットされる。
class BinaryView : public QWidget {
  Q_OBJECT

public:
  explicit BinaryView(QWidget* parent = nullptr);

  // 失敗時 (open エラー) は false。
  bool loadFile(const QString& filePath);

  // 表示中ファイルをクリア。
  void clearContent();

private:
  void setupUi();
  void render();
  void syncFromSettings();
  void rebuildEncodingItems();

  // 大きすぎるファイルの読み込みを抑制するためのソフト上限 (bytes)。
  // 超過分は読み飛ばし、末尾に注記行を付与する。
  static constexpr qint64 kMaxBytes = 8 * 1024 * 1024;

  QComboBox*          m_unitCombo     = nullptr;
  QComboBox*          m_endianCombo   = nullptr;
  QComboBox*          m_encodingCombo = nullptr;
  QPlainTextEdit*     m_textArea      = nullptr;
  AddressHighlighter* m_addressHighlighter = nullptr;

  QString    m_filePath;
  QByteArray m_data;
  qint64     m_totalSize = 0;
  qint64     m_loadedSize = 0;

  // 表示に使う実効設定 (ローカル上書き)。Settings 変更時に再同期される。
  BinaryViewerUnit   m_unit     = BinaryViewerUnit::Byte1;
  BinaryViewerEndian m_endian   = BinaryViewerEndian::Little;
  QString            m_encoding = QStringLiteral("UTF-8");
};

} // namespace Farman
