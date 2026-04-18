#include "MainWindow.h"
#include "FileManagerPanel.h"
#include "FileListPane.h"
#include "ViewerPanel.h"
#include "SettingsDialog.h"
#include "../keybinding/ICommand.h"
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QKeyEvent>
#include <QTableView>
#include <QApplication>

namespace Farman {

MainWindow::MainWindow(QWidget* parent)
  : QMainWindow(parent)
  , m_stack(nullptr)
  , m_fileManagerPanel(nullptr)
  , m_viewerPanel(nullptr) {

  setupUi();

  // Load settings
  Settings::instance().load();

  // Register commands
  registerCommands();

  // Load keybindings
  KeyBindingManager::instance().loadFromSettings();

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
        // Try to route through KeyBindingManager first
        QKeySequence keySeq(keyEvent->key() | keyEvent->modifiers());
        QString commandId = KeyBindingManager::instance().commandFor(keySeq);

        if (!commandId.isEmpty()) {
          // Execute the command through the registry
          if (CommandRegistry::instance().execute(commandId)) {
            return true;
          }
        }

        // Fall back to FileManagerPanel's handleKeyEvent for unbound keys
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

void MainWindow::registerCommands() {
  auto& registry = CommandRegistry::instance();

  // Navigation commands
  registry.registerCommand(std::make_shared<LambdaCommand>(
    "navigate.up",
    "Navigate Up",
    [this]() {
      QKeyEvent event(QEvent::KeyPress, Qt::Key_Up, Qt::NoModifier);
      m_fileManagerPanel->handleKeyEvent(&event);
    },
    "navigation"
  ));

  registry.registerCommand(std::make_shared<LambdaCommand>(
    "navigate.down",
    "Navigate Down",
    [this]() {
      QKeyEvent event(QEvent::KeyPress, Qt::Key_Down, Qt::NoModifier);
      m_fileManagerPanel->handleKeyEvent(&event);
    },
    "navigation"
  ));

  registry.registerCommand(std::make_shared<LambdaCommand>(
    "navigate.left",
    "Navigate Left",
    [this]() {
      QKeyEvent event(QEvent::KeyPress, Qt::Key_Left, Qt::NoModifier);
      m_fileManagerPanel->handleKeyEvent(&event);
    },
    "navigation"
  ));

  registry.registerCommand(std::make_shared<LambdaCommand>(
    "navigate.right",
    "Navigate Right",
    [this]() {
      QKeyEvent event(QEvent::KeyPress, Qt::Key_Right, Qt::NoModifier);
      m_fileManagerPanel->handleKeyEvent(&event);
    },
    "navigation"
  ));

  registry.registerCommand(std::make_shared<LambdaCommand>(
    "navigate.home",
    "Navigate Home",
    [this]() {
      QKeyEvent event(QEvent::KeyPress, Qt::Key_Home, Qt::NoModifier);
      m_fileManagerPanel->handleKeyEvent(&event);
    },
    "navigation"
  ));

  registry.registerCommand(std::make_shared<LambdaCommand>(
    "navigate.end",
    "Navigate End",
    [this]() {
      QKeyEvent event(QEvent::KeyPress, Qt::Key_End, Qt::NoModifier);
      m_fileManagerPanel->handleKeyEvent(&event);
    },
    "navigation"
  ));

  registry.registerCommand(std::make_shared<LambdaCommand>(
    "navigate.pageup",
    "Navigate Page Up",
    [this]() {
      QKeyEvent event(QEvent::KeyPress, Qt::Key_PageUp, Qt::NoModifier);
      m_fileManagerPanel->handleKeyEvent(&event);
    },
    "navigation"
  ));

  registry.registerCommand(std::make_shared<LambdaCommand>(
    "navigate.pagedown",
    "Navigate Page Down",
    [this]() {
      QKeyEvent event(QEvent::KeyPress, Qt::Key_PageDown, Qt::NoModifier);
      m_fileManagerPanel->handleKeyEvent(&event);
    },
    "navigation"
  ));

  registry.registerCommand(std::make_shared<LambdaCommand>(
    "navigate.enter",
    "Enter Directory or Open File",
    [this]() {
      QKeyEvent event(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
      m_fileManagerPanel->handleKeyEvent(&event);
    },
    "navigation"
  ));

  registry.registerCommand(std::make_shared<LambdaCommand>(
    "navigate.parent",
    "Navigate to Parent Directory",
    [this]() {
      QKeyEvent event(QEvent::KeyPress, Qt::Key_Backspace, Qt::NoModifier);
      m_fileManagerPanel->handleKeyEvent(&event);
    },
    "navigation"
  ));

  // Selection commands
  registry.registerCommand(std::make_shared<LambdaCommand>(
    "select.toggle",
    "Toggle Selection",
    [this]() {
      QKeyEvent event(QEvent::KeyPress, Qt::Key_Space, Qt::NoModifier);
      m_fileManagerPanel->handleKeyEvent(&event);
    },
    "selection"
  ));

  registry.registerCommand(std::make_shared<LambdaCommand>(
    "select.toggle_and_down",
    "Toggle Selection and Move Down",
    [this]() {
      QKeyEvent event(QEvent::KeyPress, Qt::Key_Insert, Qt::NoModifier);
      m_fileManagerPanel->handleKeyEvent(&event);
    },
    "selection"
  ));

  registry.registerCommand(std::make_shared<LambdaCommand>(
    "select.invert",
    "Invert Selection",
    [this]() {
      QKeyEvent event(QEvent::KeyPress, Qt::Key_Asterisk, Qt::NoModifier);
      m_fileManagerPanel->handleKeyEvent(&event);
    },
    "selection"
  ));

  registry.registerCommand(std::make_shared<LambdaCommand>(
    "select.all",
    "Select All",
    [this]() {
      QKeyEvent event(QEvent::KeyPress, Qt::Key_A, Qt::ControlModifier);
      m_fileManagerPanel->handleKeyEvent(&event);
    },
    "selection"
  ));

  // Pane commands
  registry.registerCommand(std::make_shared<LambdaCommand>(
    "pane.switch",
    "Switch Pane",
    [this]() {
      QKeyEvent event(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier);
      m_fileManagerPanel->handleKeyEvent(&event);
    },
    "pane"
  ));

  registry.registerCommand(std::make_shared<LambdaCommand>(
    "pane.toggle_single",
    "Toggle Single Pane Mode",
    [this]() {
      m_fileManagerPanel->togglePaneMode();
    },
    "pane"
  ));

  // File operation commands
  registry.registerCommand(std::make_shared<LambdaCommand>(
    "file.copy",
    "Copy",
    [this]() {
      m_fileManagerPanel->copySelectedFiles();
    },
    "file"
  ));

  registry.registerCommand(std::make_shared<LambdaCommand>(
    "file.move",
    "Move",
    [this]() {
      m_fileManagerPanel->moveSelectedFiles();
    },
    "file"
  ));

  registry.registerCommand(std::make_shared<LambdaCommand>(
    "file.delete",
    "Delete",
    [this]() {
      m_fileManagerPanel->deleteSelectedFiles();
    },
    "file"
  ));

  registry.registerCommand(std::make_shared<LambdaCommand>(
    "file.mkdir",
    "Make Directory",
    [this]() {
      m_fileManagerPanel->createDirectory();
    },
    "file"
  ));

  // Application commands
  registry.registerCommand(std::make_shared<LambdaCommand>(
    "app.quit",
    "Quit",
    [this]() {
      QApplication::quit();
    },
    "application"
  ));

  registry.registerCommand(std::make_shared<LambdaCommand>(
    "app.settings",
    "Settings",
    [this]() {
      showSettingsDialog();
    },
    "application"
  ));

  registry.registerCommand(std::make_shared<LambdaCommand>(
    "view.file",
    "View File",
    [this]() {
      QKeyEvent event(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
      m_fileManagerPanel->handleKeyEvent(&event);
    },
    "view"
  ));
}

void MainWindow::showSettingsDialog() {
  SettingsDialog* dialog = new SettingsDialog(this);
  connect(dialog, &SettingsDialog::settingsChanged, this, &MainWindow::onSettingsChanged);
  dialog->exec();
  delete dialog;
}

void MainWindow::onSettingsChanged() {
  // Settings have been changed and saved
  // The Settings singleton will emit its own settingsChanged signal
  // which can be connected to by other components if needed
}

} // namespace Farman
