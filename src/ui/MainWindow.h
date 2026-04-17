#pragma once

#include <QMainWindow>
#include "types.h"

class QSplitter;

namespace Farman {

class FileListPane;

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit MainWindow(QWidget* parent = nullptr);
  ~MainWindow() override;

protected:
  void keyPressEvent(QKeyEvent* event) override;
  bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
  void onLeftPaneCurrentChanged(const QModelIndex& current, const QModelIndex& previous);
  void onRightPaneCurrentChanged(const QModelIndex& current, const QModelIndex& previous);
  void onLeftFolderButtonClicked();
  void onRightFolderButtonClicked();

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

  FileListPane* activePane() const;
  void setActivePane(PaneType pane);
  void setSinglePaneMode(bool single);

  QSplitter* m_splitter;
  FileListPane* m_leftPane;
  FileListPane* m_rightPane;

  PaneType m_activePane;
  bool m_singlePaneMode;
};

} // namespace Farman
