#pragma once

#include <QByteArray>
#include <QString>
#include <QWidget>

class QComboBox;
class QCheckBox;

namespace Farman {

class TextEditArea;

// テキストファイル表示用ウィジェット。
//   - 上部のツールバーで Encoding / Line Numbers / Word Wrap をその場で上書き可能
//     (上書きは Settings に保存されない)
//   - フォント / カラー / 行番号エリアの色は Settings から取得する
//   - Settings::settingsChanged でローカルの上書きはリセットされる
class TextView : public QWidget {
  Q_OBJECT

public:
  explicit TextView(QWidget* parent = nullptr);

  // 失敗時 (open エラー) は false。
  bool loadFile(const QString& filePath);

  // 表示中ファイルをクリア。
  void clearContent();

  // ステータスバー表示用の要約 ("1234 lines · UTF-8 · 56 KB" 等)。
  QString statusInfo() const;

  // 内部 TextEditArea が paintEvent から呼び出すアクセサ群。
  bool   showLineNumbers() const { return m_showLineNumbers; }
  QColor lineNumberForeground() const;
  QColor lineNumberBackground() const;

private:
  void setupUi();
  void syncFromSettings();
  void applyEditAreaSettings();
  void reloadFromBuffer();
  static QString decodeBytes(const QByteArray& data, const QString& encoding);

  TextEditArea* m_editArea  = nullptr;
  QComboBox*    m_encodingCombo     = nullptr;
  QCheckBox*    m_lineNumbersCheck  = nullptr;
  QCheckBox*    m_wordWrapCheck     = nullptr;

  QString    m_filePath;
  QByteArray m_data;

  // 表示に使う実効設定 (ローカル上書き)。Settings 変更で再同期される。
  QString m_encoding         = QStringLiteral("UTF-8");
  bool    m_showLineNumbers  = true;
  bool    m_wordWrap         = false;
};

} // namespace Farman
