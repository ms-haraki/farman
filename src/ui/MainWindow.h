#pragma once

#include <QMainWindow>

class QTableView;

namespace Farman {

class FileListModel;

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit MainWindow(QWidget* parent = nullptr);
  ~MainWindow() override;

protected:
  void keyPressEvent(QKeyEvent* event) override;

private:
  void setupUi();
  void loadInitialPath();
  void handleEnterKey();
  void handleBackspaceKey();
  void handleSpaceKey();

  QTableView* m_tableView;
  FileListModel* m_model;
};

} // namespace Farman
