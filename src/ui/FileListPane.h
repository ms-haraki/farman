#pragma once

#include <QWidget>

class QLabel;
class QToolButton;
class QTableView;

namespace Farman {

class FileListModel;
class FileListDelegate;
class ClickableLabel;

class FileListPane : public QWidget {
  Q_OBJECT

public:
  explicit FileListPane(QWidget* parent = nullptr);
  ~FileListPane() override;

  // アクセサ
  QTableView* view() const { return m_view; }
  FileListModel* model() const { return m_model; }
  FileListDelegate* delegate() const { return m_delegate; }

  // パス操作
  QString currentPath() const;
  bool setPath(const QString& path);

  // アクティブ状態
  void setActive(bool active);

  // 現在のソート・フィルタ条件をフッタに表示するラベルを更新する
  void refreshSortFilterStatus();

  // 設定からパス表示のカラー等を再適用する
  void refreshAppearance();

signals:
  void folderButtonClicked();
  void currentChanged(const QModelIndex& current, const QModelIndex& previous);

public:
  // 現在のパスのブックマーク登録状態を切り替える。
  // - 登録済みなら即削除。
  // - 未登録なら名前入力ダイアログを出し、OK で追加、キャンセルで何もしない。
  void toggleBookmarkForCurrentPath();

private slots:
  void onFolderButtonClicked();
  void onBookmarkButtonClicked();
  void onCurrentChanged(const QModelIndex& current, const QModelIndex& previous);
  void onHeaderClicked(int section);
  // 現在のパスがブックマーク済みかどうかを indicator に反映する
  void refreshBookmarkIndicator();

private:
  void setupUi();

  QLabel* m_addressLabel;
  ClickableLabel* m_bookmarkLabel;
  QToolButton* m_folderButton;
  QTableView* m_view;
  QLabel* m_sortFilterStatusLabel;
  FileListModel* m_model;
  FileListDelegate* m_delegate;
};

} // namespace Farman
