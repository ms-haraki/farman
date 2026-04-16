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
  // Ctrl+A for select all
  if (event->modifiers() & Qt::ControlModifier && event->key() == Qt::Key_A) {
    handleSelectAllKey();
    event->accept();
    return;
  }

  switch (event->key()) {
    case Qt::Key_Return:
    case Qt::Key_Enter:
      handleEnterKey();
      event->accept();
      return;

    case Qt::Key_Backspace:
      handleBackspaceKey();
      event->accept();
      return;

    case Qt::Key_Space:
      handleSpaceKey();
      event->accept();
      return;

    case Qt::Key_Insert:
      handleInsertKey();
      event->accept();
      return;

    case Qt::Key_Asterisk:
      handleAsteriskKey();
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

void MainWindow::handleBackspaceKey() {
  QString currentPath = m_model->currentPath();
  if (currentPath.isEmpty()) {
    return;
  }

  QDir dir(currentPath);
  if (!dir.cdUp()) {
    // ルートディレクトリにいる場合は何もしない
    return;
  }

  QString parentPath = dir.absolutePath();
  if (m_model->setPath(parentPath)) {
    setWindowTitle(QString("farman - %1").arg(parentPath));
  }
}

void MainWindow::handleSpaceKey() {
  QModelIndex currentIndex = m_tableView->currentIndex();
  if (!currentIndex.isValid()) {
    return;
  }

  int row = currentIndex.row();
  m_model->toggleSelected(row);

  // カーソルを次の行に移動
  int nextRow = row + 1;
  if (nextRow < m_model->rowCount()) {
    QModelIndex nextIndex = m_model->index(nextRow, 0);
    m_tableView->setCurrentIndex(nextIndex);
  }
}

void MainWindow::handleInsertKey() {
  QModelIndex currentIndex = m_tableView->currentIndex();
  if (!currentIndex.isValid()) {
    return;
  }

  int row = currentIndex.row();
  m_model->toggleSelected(row);

  // カーソルは移動しない（Space との違い）
}

void MainWindow::handleAsteriskKey() {
  // 選択を反転
  m_model->invertSelection();
}

void MainWindow::handleSelectAllKey() {
  // 全選択
  m_model->setSelectedAll(true);
}

} // namespace Farman
