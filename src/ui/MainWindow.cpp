#include "MainWindow.h"
#include "FileListPane.h"
#include "model/FileListModel.h"
#include "core/FileItem.h"
#include <QTableView>
#include <QVBoxLayout>
#include <QSplitter>
#include <QFileDialog>
#include <QDir>
#include <QKeyEvent>
#include <QEvent>

namespace Farman {

MainWindow::MainWindow(QWidget* parent)
  : QMainWindow(parent)
  , m_splitter(nullptr)
  , m_leftPane(nullptr)
  , m_rightPane(nullptr)
  , m_activePane(PaneType::Left)
  , m_singlePaneMode(false) {

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
  m_leftPane = new FileListPane(this);
  m_leftPane->view()->installEventFilter(this);
  connect(m_leftPane, &FileListPane::currentChanged, this, &MainWindow::onLeftPaneCurrentChanged);
  connect(m_leftPane, &FileListPane::folderButtonClicked, this, &MainWindow::onLeftFolderButtonClicked);
  m_splitter->addWidget(m_leftPane);

  // ===== Right Pane =====
  m_rightPane = new FileListPane(this);
  m_rightPane->view()->installEventFilter(this);
  connect(m_rightPane, &FileListPane::currentChanged, this, &MainWindow::onRightPaneCurrentChanged);
  connect(m_rightPane, &FileListPane::folderButtonClicked, this, &MainWindow::onRightFolderButtonClicked);
  m_splitter->addWidget(m_rightPane);

  // Splitterのサイズを均等に
  m_splitter->setSizes(QList<int>() << 600 << 600);
}

void MainWindow::loadInitialPath() {
  QString homePath = QDir::homePath();

  m_leftPane->setPath(homePath);
  m_rightPane->setPath(homePath);

  // 左ペインをアクティブに
  setActivePane(PaneType::Left);

  setWindowTitle(QString("farman - L:%1 | R:%2")
    .arg(m_leftPane->currentPath())
    .arg(m_rightPane->currentPath()));
}

bool MainWindow::eventFilter(QObject* obj, QEvent* event) {
  if ((obj == m_leftPane->view() || obj == m_rightPane->view()) && event->type() == QEvent::KeyPress) {
    QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);

    // Ctrl+U (Windows/Linux) or Cmd+U (Mac) で1ペイン/2ペイン切り替え
    if ((keyEvent->modifiers() & Qt::ControlModifier || keyEvent->modifiers() & Qt::MetaModifier)
        && keyEvent->key() == Qt::Key_U) {
      togglePaneMode();
      return true;
    }

    // Tabキーでペイン切り替え（2ペイン表示時のみ）
    if (keyEvent->key() == Qt::Key_Tab && !m_singlePaneMode) {
      handleTabKey();
      return true;
    }

    // 左キーの処理
    if (keyEvent->key() == Qt::Key_Left) {
      if (m_singlePaneMode) {
        // シングルペインモード: 親ディレクトリに移動
        handleBackspaceKey();
        return true;
      } else {
        // 2ペインモード
        if (m_activePane == PaneType::Left) {
          // 左ペインで←キー: 親ディレクトリに移動
          handleBackspaceKey();
          return true;
        } else {
          // 右ペインで←キー: 左ペインに切り替え
          setActivePane(PaneType::Left);
          return true;
        }
      }
    }

    // 右キーの処理
    if (keyEvent->key() == Qt::Key_Right) {
      if (m_singlePaneMode) {
        // シングルペインモード: 何もしない（または将来的に別の機能を割り当て）
        return true;
      } else {
        // 2ペインモード
        if (m_activePane == PaneType::Right) {
          // 右ペインで→キー: 親ディレクトリに移動
          handleBackspaceKey();
          return true;
        } else {
          // 左ペインで→キー: 右ペインに切り替え
          setActivePane(PaneType::Right);
          return true;
        }
      }
    }

    // Ctrl+A (Windows/Linux) or Cmd+A (Mac) for select all
    if ((keyEvent->modifiers() & Qt::ControlModifier || keyEvent->modifiers() & Qt::MetaModifier)
        && keyEvent->key() == Qt::Key_A) {
      handleSelectAllKey();
      return true;
    }

    FileListPane* pane = activePane();
    FileListModel* model = pane->model();

    switch (keyEvent->key()) {
      case Qt::Key_Up: {
        QModelIndex current = pane->view()->currentIndex();
        if (current.isValid() && current.row() > 0) {
          pane->view()->setCurrentIndex(model->index(current.row() - 1, 0));
        }
        return true;
      }

      case Qt::Key_Down: {
        QModelIndex current = pane->view()->currentIndex();
        if (current.isValid() && current.row() < model->rowCount() - 1) {
          pane->view()->setCurrentIndex(model->index(current.row() + 1, 0));
        }
        return true;
      }

      case Qt::Key_Home: {
        if (model->rowCount() > 0) {
          pane->view()->setCurrentIndex(model->index(0, 0));
        }
        return true;
      }

      case Qt::Key_End: {
        int lastRow = model->rowCount() - 1;
        if (lastRow >= 0) {
          pane->view()->setCurrentIndex(model->index(lastRow, 0));
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

void MainWindow::onLeftPaneCurrentChanged(const QModelIndex& current, const QModelIndex& previous) {
  Q_UNUSED(current);
  Q_UNUSED(previous);
  m_leftPane->view()->viewport()->update();
}

void MainWindow::onRightPaneCurrentChanged(const QModelIndex& current, const QModelIndex& previous) {
  Q_UNUSED(current);
  Q_UNUSED(previous);
  m_rightPane->view()->viewport()->update();
}

void MainWindow::handleEnterKey() {
  FileListPane* pane = activePane();
  FileListModel* model = pane->model();

  QModelIndex currentIndex = pane->view()->currentIndex();
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
    if (pane->setPath(newPath)) {
      // ウィンドウタイトルを更新
      setWindowTitle(QString("farman - L:%1 | R:%2")
        .arg(m_leftPane->currentPath())
        .arg(m_rightPane->currentPath()));
    }
  } else {
    // ファイルの場合は将来ビュアーで開く
    // TODO: ViewerDispatcher を使ってファイルを開く
  }
}

void MainWindow::handleBackspaceKey() {
  FileListPane* pane = activePane();
  FileListModel* model = pane->model();

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
  if (pane->setPath(parentPath)) {
    // ウィンドウタイトルを更新
    setWindowTitle(QString("farman - L:%1 | R:%2")
      .arg(m_leftPane->currentPath())
      .arg(m_rightPane->currentPath()));
  }
}

void MainWindow::handleSpaceKey() {
  FileListPane* pane = activePane();
  FileListModel* model = pane->model();

  QModelIndex currentIndex = pane->view()->currentIndex();
  if (!currentIndex.isValid()) {
    return;
  }

  int row = currentIndex.row();
  model->toggleSelected(row);

  // カーソルを次の行に移動
  int nextRow = row + 1;
  if (nextRow < model->rowCount()) {
    QModelIndex nextIndex = model->index(nextRow, 0);
    pane->view()->setCurrentIndex(nextIndex);
  }
}

void MainWindow::handleInsertKey() {
  FileListPane* pane = activePane();
  FileListModel* model = pane->model();

  QModelIndex currentIndex = pane->view()->currentIndex();
  if (!currentIndex.isValid()) {
    return;
  }

  int row = currentIndex.row();
  model->toggleSelected(row);

  // カーソルは移動しない（Space との違い）
}

void MainWindow::handleAsteriskKey() {
  FileListModel* model = activePane()->model();
  // 選択を反転
  model->invertSelection();
}

void MainWindow::handleSelectAllKey() {
  FileListModel* model = activePane()->model();
  // 全て選択されている場合は全解除、それ以外は全選択
  if (model->isAllSelected()) {
    model->setSelectedAll(false);
  } else {
    model->setSelectedAll(true);
  }
}

void MainWindow::handleTabKey() {
  // アクティブペインを切り替え（2ペイン表示時のみ）
  if (!m_singlePaneMode) {
    if (m_activePane == PaneType::Left) {
      setActivePane(PaneType::Right);
    } else {
      setActivePane(PaneType::Left);
    }
  }
}

void MainWindow::togglePaneMode() {
  setSinglePaneMode(!m_singlePaneMode);
}

void MainWindow::setSinglePaneMode(bool single) {
  m_singlePaneMode = single;

  if (single) {
    // 1ペインモード: 非アクティブなペインを非表示
    if (m_activePane == PaneType::Left) {
      m_rightPane->hide();
      m_leftPane->show();
    } else {
      m_leftPane->hide();
      m_rightPane->show();
    }
  } else {
    // 2ペインモード: 両方のペインを表示
    m_leftPane->show();
    m_rightPane->show();
  }
}

FileListPane* MainWindow::activePane() const {
  return (m_activePane == PaneType::Left) ? m_leftPane : m_rightPane;
}

void MainWindow::setActivePane(PaneType pane) {
  m_activePane = pane;

  // ペインのアクティブ状態を更新
  if (pane == PaneType::Left) {
    m_leftPane->setActive(true);
    m_rightPane->setActive(false);
  } else {
    m_leftPane->setActive(false);
    m_rightPane->setActive(true);
  }

  // アクティブペインにフォーカスを設定
  activePane()->view()->setFocus();
}

void MainWindow::onLeftFolderButtonClicked() {
  QString currentPath = m_leftPane->currentPath();
  QString selectedPath = QFileDialog::getExistingDirectory(
    this,
    tr("Select Folder"),
    currentPath,
    QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
  );

  if (!selectedPath.isEmpty() && selectedPath != currentPath) {
    if (m_leftPane->setPath(selectedPath)) {
      // ウィンドウタイトルを更新
      setWindowTitle(QString("farman - L:%1 | R:%2")
        .arg(m_leftPane->currentPath())
        .arg(m_rightPane->currentPath()));
    }
  }
}

void MainWindow::onRightFolderButtonClicked() {
  QString currentPath = m_rightPane->currentPath();
  QString selectedPath = QFileDialog::getExistingDirectory(
    this,
    tr("Select Folder"),
    currentPath,
    QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
  );

  if (!selectedPath.isEmpty() && selectedPath != currentPath) {
    if (m_rightPane->setPath(selectedPath)) {
      // ウィンドウタイトルを更新
      setWindowTitle(QString("farman - L:%1 | R:%2")
        .arg(m_leftPane->currentPath())
        .arg(m_rightPane->currentPath()));
    }
  }
}

} // namespace Farman
