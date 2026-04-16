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

private:
  void setupUi();
  void loadInitialPath();

  QTableView* m_tableView;
  FileListModel* m_model;
};

} // namespace Farman
