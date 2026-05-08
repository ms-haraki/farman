#pragma once

#include <QDialog>

class QLineEdit;
class QTableWidget;

namespace Farman {

// 全コマンドのキーバインド一覧を表示するモードレスダイアログ。
// `?` キーでトグル表示する想定。読み取り専用 (編集は Settings →
// Keybindings タブで行う)。ヘッダにインクリメンタル検索ボックスがあり、
// キー名 / コマンド名 / ID で絞り込める。
class ShortcutListDialog : public QDialog {
  Q_OBJECT

public:
  explicit ShortcutListDialog(QWidget* parent = nullptr);

  // KeyBindingManager / CommandRegistry の現状を再走査して
  // テーブルを作り直す。Settings::settingsChanged で呼ぶ。
  void rebuild();

protected:
  void keyPressEvent(QKeyEvent* event) override;
  void showEvent(QShowEvent* event) override;

private:
  void setupUi();
  // 検索ボックスの内容に従ってテーブルの各行を表示/非表示にする。
  // 空のカテゴリ見出しは隠す。
  void applyFilter(const QString& filter);

  QLineEdit*    m_searchEdit = nullptr;
  QTableWidget* m_table      = nullptr;
};

} // namespace Farman
