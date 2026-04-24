#pragma once

#include <QWidget>
#include "types.h"
#include "core/DirectoryHistory.h"

class QSplitter;
class QKeyEvent;

namespace Farman {

class FileListPane;
class FileListModel;

class FileManagerPanel : public QWidget {
  Q_OBJECT

public:
  explicit FileManagerPanel(QWidget* parent = nullptr);
  ~FileManagerPanel() override;

  // 初期化
  void loadInitialPath();
  void applySettings();  // 設定を両ペインに適用

  // アクセサ
  FileListPane* activePane() const;
  FileListPane* leftPane() const { return m_leftPane; }
  FileListPane* rightPane() const { return m_rightPane; }
  QString currentPath() const;
  QString leftPath() const;
  QString rightPath() const;

  // ペイン操作
  void setActivePane(PaneType pane);
  void setSinglePaneMode(bool single);
  bool isSinglePaneMode() const { return m_singlePaneMode; }
  void togglePaneMode();

  // キーイベント処理
  bool handleKeyEvent(QKeyEvent* event);

  // ファイル操作
  void copySelectedFiles();
  void moveSelectedFiles();
  void deleteSelectedFiles();
  void createDirectory();
  void createFile();
  void renameItem();
  void changeAttributes();

  // アクティブペインのソート・フィルタ編集ダイアログを開く
  void openSortFilterDialog();

  // 任意のパスへアクティブペインを遷移させる（失敗時は false）
  bool navigateActivePaneTo(const QString& path);

  // ── ディレクトリ履歴 ─────────────────────
  DirectoryHistory&       history(PaneType pane);
  const DirectoryHistory& history(PaneType pane) const;

signals:
  void pathChanged(const QString& leftPath, const QString& rightPath);
  void fileActivated(const QString& filePath);

private slots:
  void onLeftPaneCurrentChanged(const QModelIndex& current, const QModelIndex& previous);
  void onRightPaneCurrentChanged(const QModelIndex& current, const QModelIndex& previous);
  void onLeftFolderButtonClicked();
  void onRightFolderButtonClicked();

private:
  void setupUi();
  void handleEnterKey();
  void handleBackspaceKey();
  void handleSpaceKey();
  void handleInsertKey();
  void handleAsteriskKey();
  void handleSelectAllKey();
  void handleTabKey();
  void updatePathSignal();

  // ペインのパスに対応する保存済み override があればそれを、なければ既定を適用する
  void applyPathSortFilter(PaneType paneType);
  // ペインのパス遷移を一元化するヘルパ（applyPathSortFilter 適用まで面倒を見る）
  bool navigatePane(PaneType paneType, const QString& path);

  QSplitter* m_splitter;
  FileListPane* m_leftPane;
  FileListPane* m_rightPane;

  PaneType m_activePane;
  bool m_singlePaneMode;

  DirectoryHistory m_leftHistory;
  DirectoryHistory m_rightHistory;
};

} // namespace Farman
