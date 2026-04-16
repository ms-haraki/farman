#pragma once

#include <QMainWindow>

class QTableView;

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

private:
  void setupUi();
  void loadInitialPath();
  void handleEnterKey();
  void handleBackspaceKey();
  void handleSpaceKey();
  void handleInsertKey();
  void handleAsteriskKey();
  void handleSelectAllKey();

  QTableView* m_tableView;
  FileListModel* m_model;
  FileListDelegate* m_delegate;
};

} // namespace Farman
