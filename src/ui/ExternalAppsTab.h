#pragma once

#include <QWidget>
#include "core/UserCommand.h"
#include "core/AppPresets.h"

class QLineEdit;
class QToolButton;
class QPushButton;
class QComboBox;

namespace Farman {

// Settings ダイアログの「外部アプリ (External Apps)」タブ。
// 組み込みの terminal / editor 2 件を専用 UI で編集する。ユーザー定義コマンドの
// 追加 / 編集 UI はフェーズ 2 で同タブに被せる予定。
class ExternalAppsTab : public QWidget {
  Q_OBJECT

public:
  explicit ExternalAppsTab(QWidget* parent = nullptr);
  ~ExternalAppsTab() override = default;

  void save();

private slots:
  void onBrowseTerminalProgram();
  void onBrowseEditorProgram();
  void onTestLaunchTerminal();
  void onTestLaunchEditor();
  // Preset コンボでプリセットが選ばれたとき、対応する Program / Arguments /
  // Working Dir フィールドを上書きする。"(Custom)" を選んだときは何もしない。
  void onTerminalPresetChanged(int index);
  void onEditorPresetChanged(int index);
  // ユーザー定義コマンド (= builtin terminal / editor 以外) の Export / Import。
  // SPEC.md L1564 の「ユーザー定義コマンドのインポート / エクスポート」を実装。
  // 組み込み 2 件は OS 依存なので対象外 (Import の Replace モードでも保持)。
  void onExportCommands();
  void onImportCommands();

private:
  void setupUi();
  void loadSettings();

  // Preset コンボに「(Custom)」+ 検出済みプリセットを並べる。
  // userData には preset.id を入れる ("(Custom)" は空文字)。
  void populatePresetCombo(QComboBox* combo, const QList<AppPreset>& presets);

  // フィールドの textEdited を受けて、コンボを「(Custom)」に戻す。
  // ユーザーが手で書き換えた瞬間にプリセット表示が嘘になるのを防ぐ。
  void switchTerminalToCustom();
  void switchEditorToCustom();

  // Terminal
  QComboBox*   m_terminalPresetCombo    = nullptr;
  QLineEdit*   m_terminalProgramEdit    = nullptr;
  QToolButton* m_terminalProgramBrowse  = nullptr;
  QLineEdit*   m_terminalArgsEdit       = nullptr;
  QLineEdit*   m_terminalWorkingDirEdit = nullptr;
  QPushButton* m_terminalTestButton     = nullptr;

  // Text Editor
  QComboBox*   m_editorPresetCombo      = nullptr;
  QLineEdit*   m_editorProgramEdit      = nullptr;
  QToolButton* m_editorProgramBrowse    = nullptr;
  QLineEdit*   m_editorArgsEdit         = nullptr;
  QLineEdit*   m_editorWorkingDirEdit   = nullptr;
  QPushButton* m_editorTestButton       = nullptr;

  // タブ構築時に検出したプリセット一覧 (Combo の userData の id から逆引きする
  // ためにスナップショット保持)。
  QList<AppPreset> m_terminalPresets;
  QList<AppPreset> m_editorPresets;

  // Settings::userCommands() のうち builtin 以外のエントリは UI に出さないが、
  // save() の往復で消えないようロード時に控えておき、save 時に再付加する。
  QList<UserCommand> m_nonBuiltinUserCommands;
};

} // namespace Farman
