#pragma once

#include <QDialog>

class QTableWidget;

namespace Farman {

// 全コマンドのキーバインド一覧を表示するモードレスダイアログ。
// `?` キーでトグル表示する想定。読み取り専用 (編集は Settings →
// Keybindings タブで行う)。
class ShortcutListDialog : public QDialog {
  Q_OBJECT

public:
  explicit ShortcutListDialog(QWidget* parent = nullptr);

  // KeyBindingManager / CommandRegistry の現状を再走査して
  // テーブルを作り直す。Settings::settingsChanged で呼ぶ。
  void rebuild();

protected:
  void keyPressEvent(QKeyEvent* event) override;

private:
  void setupUi();

  QTableWidget* m_table = nullptr;
};

} // namespace Farman
