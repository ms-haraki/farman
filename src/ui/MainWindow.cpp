#include "MainWindow.h"
#include "FileListDelegate.h"
#include "model/FileListModel.h"
#include "core/FileItem.h"
#include <QTableView>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QWidget>
#include <QDir>
#include <QKeyEvent>
#include <QEvent>

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
  m_tableView->setSelectionMode(QAbstractItemView::NoSelection);  // QTableView自体の選択機能を無効化
  m_tableView->setAlternatingRowColors(true);
  m_tableView->horizontalHeader()->setStretchLastSection(true);
  m_tableView->verticalHeader()->setVisible(false);

  layout->addWidget(m_tableView);

  // Model
  m_model = new FileListModel(this);
  m_tableView->setModel(m_model);

  // Delegate（カーソル表示のカスタマイズ）
  m_delegate = new FileListDelegate(this);
  m_tableView->setItemDelegate(m_delegate);

  // イベントフィルターをインストール（キー操作をキャッチするため）
  m_tableView->installEventFilter(this);

  // currentChanged シグナルを接続（カーソル移動時に行全体を再描画）
  connect(m_tableView->selectionModel(), &QItemSelectionModel::currentChanged,
          this, &MainWindow::onCurrentChanged);

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

  // 初期カーソル位置を設定してフォーカス
  if (m_model->rowCount() > 0) {
    m_tableView->setCurrentIndex(m_model->index(0, 0));
  }
  m_tableView->setFocus();
}

bool MainWindow::eventFilter(QObject* obj, QEvent* event) {
  if (obj == m_tableView && event->type() == QEvent::KeyPress) {
    QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);

    // Ctrl+A (Windows/Linux) or Cmd+A (Mac) for select all
    if ((keyEvent->modifiers() & Qt::ControlModifier || keyEvent->modifiers() & Qt::MetaModifier)
        && keyEvent->key() == Qt::Key_A) {
      handleSelectAllKey();
      return true;
    }

    switch (keyEvent->key()) {
      case Qt::Key_Up: {
        QModelIndex current = m_tableView->currentIndex();
        if (current.isValid() && current.row() > 0) {
          m_tableView->setCurrentIndex(m_model->index(current.row() - 1, 0));
        }
        return true;
      }

      case Qt::Key_Down: {
        QModelIndex current = m_tableView->currentIndex();
        if (current.isValid() && current.row() < m_model->rowCount() - 1) {
          m_tableView->setCurrentIndex(m_model->index(current.row() + 1, 0));
        }
        return true;
      }

      case Qt::Key_Home: {
        if (m_model->rowCount() > 0) {
          m_tableView->setCurrentIndex(m_model->index(0, 0));
        }
        return true;
      }

      case Qt::Key_End: {
        int lastRow = m_model->rowCount() - 1;
        if (lastRow >= 0) {
          m_tableView->setCurrentIndex(m_model->index(lastRow, 0));
        }
        return true;
      }

      case Qt::Key_Return:
      case Qt::Key_Enter:
        handleEnterKey();
        return true;

      case Qt::Key_Backspace:
        handleBackspaceKey();
        return true;

      case Qt::Key_Space:
        handleSpaceKey();
        return true;

      case Qt::Key_Insert:
        handleInsertKey();
        return true;

      case Qt::Key_Asterisk:
        handleAsteriskKey();
        return true;

      default:
        break;
    }
  }

  return QMainWindow::eventFilter(obj, event);
}

void MainWindow::keyPressEvent(QKeyEvent* event) {
  // このメソッドはMainWindowがフォーカスを持つ場合のみ
  // 通常はeventFilterで処理される
  QMainWindow::keyPressEvent(event);
}

void MainWindow::onCurrentChanged(const QModelIndex& current, const QModelIndex& previous) {
  // カーソル位置が変わったので、旧行と新行の両方を再描画
  // これにより下線が全カラムで正しく更新される
  if (previous.isValid()) {
    // 旧行の全カラムを更新
    for (int col = 0; col < m_model->columnCount(); ++col) {
      QModelIndex idx = m_model->index(previous.row(), col);
      m_tableView->update(m_tableView->visualRect(idx));
    }
  }
  if (current.isValid()) {
    // 新行の全カラムを更新
    for (int col = 0; col < m_model->columnCount(); ++col) {
      QModelIndex idx = m_model->index(current.row(), col);
      m_tableView->update(m_tableView->visualRect(idx));
    }
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
      // カーソルを先頭に移動
      if (m_model->rowCount() > 0) {
        m_tableView->setCurrentIndex(m_model->index(0, 0));
      }
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
    // カーソルを先頭に移動
    if (m_model->rowCount() > 0) {
      m_tableView->setCurrentIndex(m_model->index(0, 0));
    }
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
  // 全て選択されている場合は全解除、それ以外は全選択
  if (m_model->isAllSelected()) {
    m_model->setSelectedAll(false);
  } else {
    m_model->setSelectedAll(true);
  }
}

} // namespace Farman
