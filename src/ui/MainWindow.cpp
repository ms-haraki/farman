#include "MainWindow.h"
#include "FileListDelegate.h"
#include "model/FileListModel.h"
#include "core/FileItem.h"
#include <QTableView>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QWidget>
#include <QDir>
#include <QKeyEvent>
#include <QEvent>

namespace Farman {

MainWindow::MainWindow(QWidget* parent)
  : QMainWindow(parent)
  , m_splitter(nullptr)
  , m_leftView(nullptr)
  , m_leftModel(nullptr)
  , m_leftDelegate(nullptr)
  , m_rightView(nullptr)
  , m_rightModel(nullptr)
  , m_rightDelegate(nullptr)
  , m_activePane(PaneType::Left) {

  setupUi();
  loadInitialPath();
}

MainWindow::~MainWindow() = default;

void MainWindow::setupUi() {
  setWindowTitle("farman - File Manager");
  resize(1200, 600);

  // Central widget
  QWidget* central = new QWidget(this);
  setCentralWidget(central);

  QVBoxLayout* layout = new QVBoxLayout(central);
  layout->setContentsMargins(0, 0, 0, 0);

  // Splitter for two panes
  m_splitter = new QSplitter(Qt::Horizontal, this);
  layout->addWidget(m_splitter);

  // ===== Left Pane =====
  m_leftView = new QTableView(this);
  m_leftView->setSelectionBehavior(QAbstractItemView::SelectRows);
  m_leftView->setSelectionMode(QAbstractItemView::NoSelection);
  m_leftView->setAlternatingRowColors(true);
  m_leftView->horizontalHeader()->setStretchLastSection(true);
  m_leftView->verticalHeader()->setVisible(false);

  m_leftModel = new FileListModel(this);
  m_leftView->setModel(m_leftModel);

  m_leftDelegate = new FileListDelegate(this);
  m_leftView->setItemDelegate(m_leftDelegate);

  m_leftView->installEventFilter(this);

  connect(m_leftView->selectionModel(), &QItemSelectionModel::currentChanged,
          this, &MainWindow::onCurrentChanged);

  m_leftView->setColumnWidth(FileListModel::Name, 250);
  m_leftView->setColumnWidth(FileListModel::Size, 100);
  m_leftView->setColumnWidth(FileListModel::Type, 80);
  m_leftView->setColumnWidth(FileListModel::LastModified, 150);

  m_splitter->addWidget(m_leftView);

  // ===== Right Pane =====
  m_rightView = new QTableView(this);
  m_rightView->setSelectionBehavior(QAbstractItemView::SelectRows);
  m_rightView->setSelectionMode(QAbstractItemView::NoSelection);
  m_rightView->setAlternatingRowColors(true);
  m_rightView->horizontalHeader()->setStretchLastSection(true);
  m_rightView->verticalHeader()->setVisible(false);

  m_rightModel = new FileListModel(this);
  m_rightView->setModel(m_rightModel);

  m_rightDelegate = new FileListDelegate(this);
  m_rightView->setItemDelegate(m_rightDelegate);

  m_rightView->installEventFilter(this);

  connect(m_rightView->selectionModel(), &QItemSelectionModel::currentChanged,
          this, &MainWindow::onCurrentChanged);

  m_rightView->setColumnWidth(FileListModel::Name, 250);
  m_rightView->setColumnWidth(FileListModel::Size, 100);
  m_rightView->setColumnWidth(FileListModel::Type, 80);
  m_rightView->setColumnWidth(FileListModel::LastModified, 150);

  m_splitter->addWidget(m_rightView);

  // Splitterのサイズを均等に
  m_splitter->setSizes(QList<int>() << 600 << 600);
}

void MainWindow::loadInitialPath() {
  // 左ペイン: ホームディレクトリから開始
  QString homePath = QDir::homePath();
  m_leftModel->setPath(homePath);
  if (m_leftModel->rowCount() > 0) {
    m_leftView->setCurrentIndex(m_leftModel->index(0, 0));
  }

  // 右ペイン: ホームディレクトリから開始
  m_rightModel->setPath(homePath);
  if (m_rightModel->rowCount() > 0) {
    m_rightView->setCurrentIndex(m_rightModel->index(0, 0));
  }

  // 左ペインをアクティブに
  setActivePane(PaneType::Left);

  setWindowTitle(QString("farman - L:%1 | R:%2").arg(homePath).arg(homePath));
}

bool MainWindow::eventFilter(QObject* obj, QEvent* event) {
  if ((obj == m_leftView || obj == m_rightView) && event->type() == QEvent::KeyPress) {
    QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);

    // Tabキーでペイン切り替え
    if (keyEvent->key() == Qt::Key_Tab) {
      handleTabKey();
      return true;
    }

    // 左ペインで→キー、または右ペインで←キーでペイン切り替え
    if (keyEvent->key() == Qt::Key_Right && m_activePane == PaneType::Left) {
      setActivePane(PaneType::Right);
      return true;
    }
    if (keyEvent->key() == Qt::Key_Left && m_activePane == PaneType::Right) {
      setActivePane(PaneType::Left);
      return true;
    }

    // Ctrl+A (Windows/Linux) or Cmd+A (Mac) for select all
    if ((keyEvent->modifiers() & Qt::ControlModifier || keyEvent->modifiers() & Qt::MetaModifier)
        && keyEvent->key() == Qt::Key_A) {
      handleSelectAllKey();
      return true;
    }

    QTableView* view = activeView();
    FileListModel* model = activeModel();

    switch (keyEvent->key()) {
      case Qt::Key_Up: {
        QModelIndex current = view->currentIndex();
        if (current.isValid() && current.row() > 0) {
          view->setCurrentIndex(model->index(current.row() - 1, 0));
        }
        return true;
      }

      case Qt::Key_Down: {
        QModelIndex current = view->currentIndex();
        if (current.isValid() && current.row() < model->rowCount() - 1) {
          view->setCurrentIndex(model->index(current.row() + 1, 0));
        }
        return true;
      }

      case Qt::Key_Home: {
        if (model->rowCount() > 0) {
          view->setCurrentIndex(model->index(0, 0));
        }
        return true;
      }

      case Qt::Key_End: {
        int lastRow = model->rowCount() - 1;
        if (lastRow >= 0) {
          view->setCurrentIndex(model->index(lastRow, 0));
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
  // カーソル位置が変わったので、ビューポート全体を再描画
  // これにより下線が全カラムで正しく更新される
  Q_UNUSED(current);
  Q_UNUSED(previous);

  // どちらのビューからのシグナルかを判定して、そのビューを再描画
  QObject* senderObj = sender();
  if (senderObj == m_leftView->selectionModel()) {
    m_leftView->viewport()->update();
  } else if (senderObj == m_rightView->selectionModel()) {
    m_rightView->viewport()->update();
  }
}

void MainWindow::handleEnterKey() {
  QTableView* view = activeView();
  FileListModel* model = activeModel();

  QModelIndex currentIndex = view->currentIndex();
  if (!currentIndex.isValid()) {
    return;
  }

  const FileItem* item = model->itemAt(currentIndex);
  if (!item) {
    return;
  }

  if (item->isDir()) {
    // ディレクトリに入る
    QString newPath = item->absolutePath();
    if (model->setPath(newPath)) {
      // カーソルを先頭に移動
      if (model->rowCount() > 0) {
        view->setCurrentIndex(model->index(0, 0));
      }
      // ウィンドウタイトルを更新
      setWindowTitle(QString("farman - L:%1 | R:%2")
        .arg(m_leftModel->currentPath())
        .arg(m_rightModel->currentPath()));
    }
  } else {
    // ファイルの場合は将来ビュアーで開く
    // TODO: ViewerDispatcher を使ってファイルを開く
  }
}

void MainWindow::handleBackspaceKey() {
  QTableView* view = activeView();
  FileListModel* model = activeModel();

  QString currentPath = model->currentPath();
  if (currentPath.isEmpty()) {
    return;
  }

  QDir dir(currentPath);
  if (!dir.cdUp()) {
    // ルートディレクトリにいる場合は何もしない
    return;
  }

  QString parentPath = dir.absolutePath();
  if (model->setPath(parentPath)) {
    // カーソルを先頭に移動
    if (model->rowCount() > 0) {
      view->setCurrentIndex(model->index(0, 0));
    }
    // ウィンドウタイトルを更新
    setWindowTitle(QString("farman - L:%1 | R:%2")
      .arg(m_leftModel->currentPath())
      .arg(m_rightModel->currentPath()));
  }
}

void MainWindow::handleSpaceKey() {
  QTableView* view = activeView();
  FileListModel* model = activeModel();

  QModelIndex currentIndex = view->currentIndex();
  if (!currentIndex.isValid()) {
    return;
  }

  int row = currentIndex.row();
  model->toggleSelected(row);

  // カーソルを次の行に移動
  int nextRow = row + 1;
  if (nextRow < model->rowCount()) {
    QModelIndex nextIndex = model->index(nextRow, 0);
    view->setCurrentIndex(nextIndex);
  }
}

void MainWindow::handleInsertKey() {
  QTableView* view = activeView();
  FileListModel* model = activeModel();

  QModelIndex currentIndex = view->currentIndex();
  if (!currentIndex.isValid()) {
    return;
  }

  int row = currentIndex.row();
  model->toggleSelected(row);

  // カーソルは移動しない（Space との違い）
}

void MainWindow::handleAsteriskKey() {
  FileListModel* model = activeModel();
  // 選択を反転
  model->invertSelection();
}

void MainWindow::handleSelectAllKey() {
  FileListModel* model = activeModel();
  // 全て選択されている場合は全解除、それ以外は全選択
  if (model->isAllSelected()) {
    model->setSelectedAll(false);
  } else {
    model->setSelectedAll(true);
  }
}

void MainWindow::handleTabKey() {
  // アクティブペインを切り替え
  if (m_activePane == PaneType::Left) {
    setActivePane(PaneType::Right);
  } else {
    setActivePane(PaneType::Left);
  }
}

QTableView* MainWindow::activeView() const {
  return (m_activePane == PaneType::Left) ? m_leftView : m_rightView;
}

FileListModel* MainWindow::activeModel() const {
  return (m_activePane == PaneType::Left) ? m_leftModel : m_rightModel;
}

FileListDelegate* MainWindow::activeDelegate() const {
  return (m_activePane == PaneType::Left) ? m_leftDelegate : m_rightDelegate;
}

void MainWindow::setActivePane(PaneType pane) {
  m_activePane = pane;

  // デリゲートのアクティブ状態を更新
  if (pane == PaneType::Left) {
    m_leftDelegate->setActive(true);
    m_rightDelegate->setActive(false);
  } else {
    m_leftDelegate->setActive(false);
    m_rightDelegate->setActive(true);
  }

  // 両方のビューを再描画してカーソル色を更新
  m_leftView->viewport()->update();
  m_rightView->viewport()->update();

  // アクティブペインにフォーカスを設定
  QTableView* view = activeView();
  view->setFocus();
}

} // namespace Farman
