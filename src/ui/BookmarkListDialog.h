#pragma once

#include <QDialog>
#include <QString>

class QTableWidget;
class QPushButton;

namespace Farman {

// ブックマーク一覧ダイアログ。ジャンプ／削除／並び替え／名前編集を行う。
// - OK（またはダブルクリック／Go）で選択中のブックマークのパスを確定して閉じる。
//   呼び出し側は exec() の戻り値が Accepted の時に selectedPath() で結果を取得。
// - それ以外のボタン（Rename/Delete/Up/Down）は BookmarkManager を直接編集する。
//   BookmarkManager::bookmarksChanged で他画面もライブ更新される。
class BookmarkListDialog : public QDialog {
  Q_OBJECT

public:
  explicit BookmarkListDialog(QWidget* parent = nullptr);
  ~BookmarkListDialog() override = default;

  // Accepted 時に取得する、ジャンプ先パス
  QString selectedPath() const { return m_selectedPath; }

private slots:
  void onGo();
  void onRename();
  void onDelete();
  void onMoveUp();
  void onMoveDown();
  void onItemDoubleClicked();
  void onSelectionChanged();
  void refreshList();

private:
  void setupUi();
  int currentRow() const;

  QTableWidget* m_table;
  QPushButton*  m_goButton;
  QPushButton*  m_renameButton;
  QPushButton*  m_deleteButton;
  QPushButton*  m_upButton;
  QPushButton*  m_downButton;
  QString       m_selectedPath;
};

} // namespace Farman
