#include "MainWindow.h"
#include "model/FileListModel.h"
#include "core/FileItem.h"
#include <QTableView>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QWidget>
#include <QDir>
#include <QKeyEvent>

namespace Farman {

MainWindow::MainWindow(QWidget* parent)
  : QMainWindow(parent)
  , m_tableView(nullptr)
  , m_model(nullptr) {

  setupUi();
  loadInitialPath();
}

MainWindow::~MainWindow() = default;

void MainWindow::setupUi() {
  setWindowTitle("farman - File Manager");
  resize(800, 600);

  // Central widget
  QWidget* central = new QWidget(this);
  setCentralWidget(central);

  QVBoxLayout* layout = new QVBoxLayout(central);
  layout->setContentsMargins(0, 0, 0, 0);

  // Table view
  m_tableView = new QTableView(this);
  m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
  m_tableView->setSelectionMode(QAbstractItemView::SingleSelection);
  m_tableView->setAlternatingRowColors(true);
  m_tableView->horizontalHeader()->setStretchLastSection(true);
  m_tableView->verticalHeader()->setVisible(false);

  layout->addWidget(m_tableView);

  // Model
  m_model = new FileListModel(this);
  m_tableView->setModel(m_model);

  // カラム幅の調整
  m_tableView->setColumnWidth(FileListModel::Name, 300);
  m_tableView->setColumnWidth(FileListModel::Size, 100);
  m_tableView->setColumnWidth(FileListModel::Type, 80);
  m_tableView->setColumnWidth(FileListModel::LastModified, 150);
}

void MainWindow::loadInitialPath() {
  // ホームディレクトリから開始
  QString homePath = QDir::homePath();
  m_model->setPath(homePath);

  setWindowTitle(QString("farman - %1").arg(homePath));
}

void MainWindow::keyPressEvent(QKeyEvent* event) {
  switch (event->key()) {
    case Qt::Key_Return:
    case Qt::Key_Enter:
      handleEnterKey();
      event->accept();
      return;

    default:
      QMainWindow::keyPressEvent(event);
      break;
  }
}

void MainWindow::handleEnterKey() {
  QModelIndex currentIndex = m_tableView->currentIndex();
  if (!currentIndex.isValid()) {
    return;
  }

  const FileItem* item = m_model->itemAt(currentIndex);
  if (!item) {
    return;
  }

  if (item->isDir()) {
    // ディレクトリに入る
    QString newPath = item->absolutePath();
    if (m_model->setPath(newPath)) {
      setWindowTitle(QString("farman - %1").arg(newPath));
    }
  } else {
    // ファイルの場合は将来ビュアーで開く
    // TODO: ViewerDispatcher を使ってファイルを開く
  }
}

} // namespace Farman
