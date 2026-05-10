#pragma once

#include <QWidget>
#include <QJsonObject>
#include <QMap>
#include <QList>
#include <QKeySequence>

class QTableWidget;
class QPushButton;
class QTableWidgetItem;
class QFrame;
class QLabel;
class QShortcut;
class QComboBox;

namespace Farman {

class KeybindingTab : public QWidget {
  Q_OBJECT

public:
  explicit KeybindingTab(QWidget* parent = nullptr);
  ~KeybindingTab() override = default;

  void save();
  void clearCurrentBinding();
  void resetToDefaults();

protected:
  bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
  void onCellDoubleClicked(int row, int column);
  void onResetToDefaults();
  void onClearBinding();
  void onRecordOk();
  void onRecordCancel();
  // 同梱プリセットの適用 (Apply ボタン)。確認ダイアログを出してから
  // KeyBindingManager の現状を全置換し、テーブルを再描画する。
  void onApplyPreset();
  // ファイルエクスポート (現在のバインドを JSON で保存)
  void onExport();
  // ファイルインポート (JSON を読んで全置換適用)
  void onImport();

private:
  void setupUi();
  void loadKeybindings();
  void updateTable();
  QString keySequenceToString(const QKeySequence& key) const;
  bool validateBinding(const QKeySequence& newKey, const QString& commandId);
  void startRecording(int row);
  // プリセット / インポート JSON → m_pendingChanges への流し込み (共通実装)。
  // 既存バインドを一旦すべて「空リスト」として候補入りさせ、JSON で出てきた
  // コマンドだけ上書きする。これにより JSON に書いていないコマンドは結果的に
  // unbound になる (= プリセットの「全置換」セマンティクス)。失敗時 false。
  bool applyBindingsToPending(const QJsonObject& obj, QString* errorMsg);

  QTableWidget* m_table;
  QPushButton*  m_resetButton;
  QPushButton*  m_clearButton;

  // ── プリセット / インポート / エクスポート ─────
  // プリセット選択はコンボの currentIndexChanged で即時プレビュー反映するので
  // Apply ボタンは存在しない。Export / Import は明示クリック必須。
  QComboBox*    m_presetCombo  = nullptr;
  QPushButton*  m_exportButton = nullptr;
  QPushButton*  m_importButton = nullptr;

  // Key recording frame (shown below table when editing)
  QFrame*       m_recordFrame;
  QLabel*       m_recordCommandLabel;
  QLabel*       m_recordCurrentKeyLabel;
  QLabel*       m_recordNewKeyLabel;
  QPushButton*  m_recordOkButton;
  QPushButton*  m_recordCancelButton;

  // ペンディング変更: コマンド ID → そのコマンドに割当てる新しいキー列。
  // - 空リスト  = 全バインドを取り除く (Clear)
  // - 1 個のみ = ユーザーが個別編集で 1 キーに置き換え
  // - 複数個   = プリセット適用などでマルチキーを保持
  // - キーが map に無い = 変更なし
  QMap<QString, QList<QKeySequence>> m_pendingChanges;

  // Track the command being edited
  QString m_editingCommandId;
  int m_editingRow;

  // Flag to indicate we're in recording mode
  bool m_inRecordMode;
  QKeySequence m_recordedKey;
};

} // namespace Farman
