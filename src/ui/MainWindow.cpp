#include "MainWindow.h"
#include "FileManagerPanel.h"
#include "FileListPane.h"
#include "ViewerPanel.h"
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QKeyEvent>
#include <QTableView>

namespace Farman {

MainWindow::MainWindow(QWidget* parent)
  : QMainWindow(parent)
  , m_stack(nullptr)
  , m_fileManagerPanel(nullptr)
  , m_viewerPanel(nullptr) {

  setupUi();

  // Show file manager and load initial path
  m_stack->setCurrentWidget(m_fileManagerPanel);
  m_fileManagerPanel->loadInitialPath();
  m_fileManagerPanel->activePane()->view()->setFocus();
}

MainWindow::~MainWindow() = default;

void MainWindow::setupUi() {
  setWindowTitle("farman - File Manager");
  resize(1200, 600);

  // Central widget with stack
  QWidget* central = new QWidget(this);
  setCentralWidget(central);

  QVBoxLayout* layout = new QVBoxLayout(central);
  layout->setContentsMargins(0, 0, 0, 0);

  m_stack = new QStackedWidget(this);
  layout->addWidget(m_stack);

  // ===== File Manager Panel =====
  m_fileManagerPanel = new FileManagerPanel(this);
  connect(m_fileManagerPanel, &FileManagerPanel::fileActivated, this, &MainWindow::onFileActivated);
  connect(m_fileManagerPanel, &FileManagerPanel::pathChanged, this, &MainWindow::onPathChanged);

  // Install event filter on both panes
  m_fileManagerPanel->leftPane()->view()->installEventFilter(this);
  m_fileManagerPanel->rightPane()->view()->installEventFilter(this);

  m_stack->addWidget(m_fileManagerPanel);

  // ===== Viewer Panel =====
  m_viewerPanel = new ViewerPanel(this);
  m_viewerPanel->installEventFilter(this);
  m_stack->addWidget(m_viewerPanel);
}

void MainWindow::showFileManager() {
  if (m_stack->currentWidget() != m_fileManagerPanel) {
    m_stack->setCurrentWidget(m_fileManagerPanel);
    m_viewerPanel->clear();

    // フォーカスをアクティブペインに戻す
    m_fileManagerPanel->activePane()->view()->setFocus();
  }
}

void MainWindow::showViewer(const QString& filePath) {
  if (m_viewerPanel->openFile(filePath)) {
    m_stack->setCurrentWidget(m_viewerPanel);
    m_viewerPanel->setFocus();
  }
}

bool MainWindow::eventFilter(QObject* obj, QEvent* event) {
  if (event->type() == QEvent::KeyPress) {
    QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);

    // ファイルマネージャーパネル表示中
    if (m_stack->currentWidget() == m_fileManagerPanel) {
      if (obj == m_fileManagerPanel->leftPane()->view() ||
          obj == m_fileManagerPanel->rightPane()->view()) {
        return m_fileManagerPanel->handleKeyEvent(keyEvent);
      }
    }
    // ビューアパネル表示中
    else if (m_stack->currentWidget() == m_viewerPanel) {
      if (obj == m_viewerPanel) {
        // Enterキーでファイルマネージャーに戻る
        if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
          showFileManager();
          return true;
        }
      }
    }
  }

  return QMainWindow::eventFilter(obj, event);
}

void MainWindow::keyPressEvent(QKeyEvent* event) {
  // ビューアパネル表示中のキーイベント
  if (m_stack->currentWidget() == m_viewerPanel) {
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
      showFileManager();
      return;
    }
  }

  QMainWindow::keyPressEvent(event);
}

void MainWindow::onFileActivated(const QString& filePath) {
  showViewer(filePath);
}

void MainWindow::onPathChanged(const QString& leftPath, const QString& rightPath) {
  setWindowTitle(QString("farman - L:%1 | R:%2")
    .arg(leftPath)
    .arg(rightPath));
}

} // namespace Farman
