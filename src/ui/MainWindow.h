#pragma once

#include <QMainWindow>
#include "types.h"

class QTableView;
class QSplitter;
class QLabel;

namespace Farman {

class FileListModel;
class FileListDelegate;

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit MainWindow(QWidget* parent = nullptr);
  ~MainWindow() override;

protected:
  void keyPressEvent(QKeyEvent* event) override;
  bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
  void onCurrentChanged(const QModelIndex& current, const QModelIndex& previous);

private:
  void setupUi();
  void loadInitialPath();
  void handleEnterKey();
  void handleBackspaceKey();
  void handleSpaceKey();
  void handleInsertKey();
  void handleAsteriskKey();
  void handleSelectAllKey();
  void handleTabKey();
  void togglePaneMode();

  // アクティブペインのビュー/モデル/デリゲートを取得
  QTableView* activeView() const;
  FileListModel* activeModel() const;
  FileListDelegate* activeDelegate() const;

  void setActivePane(PaneType pane);
  void setSinglePaneMode(bool single);

  QSplitter* m_splitter;

  // 左ペイン
  QLabel* m_leftPathLabel;
  QTableView* m_leftView;
  FileListModel* m_leftModel;
  FileListDelegate* m_leftDelegate;

  // 右ペイン
  QLabel* m_rightPathLabel;
  QTableView* m_rightView;
  FileListModel* m_rightModel;
  FileListDelegate* m_rightDelegate;

  PaneType m_activePane;
  bool m_singlePaneMode;

  void updatePathLabels();
};

} // namespace Farman
