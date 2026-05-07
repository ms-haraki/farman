#pragma once

#include <QWidget>
#include "core/UserCommand.h"

class QLineEdit;
class QToolButton;
class QPushButton;

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

private:
  void setupUi();
  void loadSettings();

  // Terminal
  QLineEdit*   m_terminalProgramEdit    = nullptr;
  QToolButton* m_terminalProgramBrowse  = nullptr;
  QLineEdit*   m_terminalArgsEdit       = nullptr;
  QLineEdit*   m_terminalWorkingDirEdit = nullptr;
  QPushButton* m_terminalTestButton     = nullptr;

  // Text Editor
  QLineEdit*   m_editorProgramEdit      = nullptr;
  QToolButton* m_editorProgramBrowse    = nullptr;
  QLineEdit*   m_editorArgsEdit         = nullptr;
  QLineEdit*   m_editorWorkingDirEdit   = nullptr;
  QPushButton* m_editorTestButton       = nullptr;

  // Settings::userCommands() のうち builtin 以外のエントリは UI に出さないが、
  // save() の往復で消えないようロード時に控えておき、save 時に再付加する。
  QList<UserCommand> m_nonBuiltinUserCommands;
};

} // namespace Farman
