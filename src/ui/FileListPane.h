#pragma once

#include <QWidget>

class QLabel;
class QToolButton;
class QTableView;
class QFileSystemWatcher;
class QTimer;

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
  // 外部からカレントディレクトリが変更されたとき (Finder 等) の遅延更新。
  // QFileSystemWatcher のイベントを debounce してから model を refresh する。
  void onExternalDirectoryChanged();

private:
  void setupUi();

  QLabel* m_addressLabel;
  ClickableLabel* m_bookmarkLabel;
  QToolButton* m_folderButton;
  QTableView* m_view;
  QLabel* m_sortFilterStatusLabel;
  FileListModel* m_model;
  FileListDelegate* m_delegate;
  // Settings の "Auto" を復元するために、構築時の Qt 既定行高を覚えておく
  int m_defaultRowHeight = 0;

  // 外部 (Finder 等) からのファイル増減を検知するためのウォッチャ。
  // setPath で監視対象を切り替える。短時間に複数イベントが発火することが
  // あるので m_refreshDebounce で間引いてから model->refresh() を呼ぶ。
  QFileSystemWatcher* m_dirWatcher     = nullptr;
  QTimer*             m_refreshDebounce = nullptr;
};

} // namespace Farman
