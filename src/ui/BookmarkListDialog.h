#pragma once

#include <QDialog>
#include <QString>
#include <QList>

class QTableWidget;
class QPushButton;

namespace Farman {

// ブックマーク一覧ダイアログ。単一テーブル内で3セクションを表示:
//   1. 固定ブックマーク (isDefault=true): Rename/Move 可、Delete 不可
//   2. "Detected locations" セパレータ + 動的検出した場所: Go のみ可
//   3. 任意ブックマーク (isDefault=false): Rename/Delete/Move 可
// OK（ダブルクリック／Go）で選択中のパスを確定して閉じる。
class BookmarkListDialog : public QDialog {
  Q_OBJECT

public:
  explicit BookmarkListDialog(QWidget* parent = nullptr);
  ~BookmarkListDialog() override = default;

  QString selectedPath() const { return m_selectedPath; }

private slots:
  void onGo();
  void onRename();
  void onDelete();
  void onMoveUp();
  void onMoveDown();
  void onSelectionChanged();
  void rebuildTable();

private:
  // 行1つ分のメタ情報
  struct RowInfo {
    enum class Type { Default, Separator, Detected, User };
    Type    type = Type::User;
    // BookmarkManager の index（Default/User 時）、Detected 配列の index（Detected 時）
    int     sourceIndex = -1;
  };

  void setupUi();
  int  currentRow() const;
  // 現在の選択行の RowInfo を返す。選択なしなら type=User, sourceIndex=-1。
  RowInfo currentRowInfo() const;

  QTableWidget*    m_table;
  QPushButton*     m_goButton;
  QPushButton*     m_renameButton;
  QPushButton*     m_deleteButton;
  QPushButton*     m_upButton;
  QPushButton*     m_downButton;
  QString          m_selectedPath;
  // 各テーブル行に対応するメタ情報。rebuildTable で同時構築。
  QList<RowInfo>   m_rowInfos;
  // Detected セクションの現在のエントリ（パス解決用）
  QList<QString>   m_detectedPaths;
};

} // namespace Farman
