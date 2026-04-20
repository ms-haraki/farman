#include "MainWindow.h"
#include "FileManagerPanel.h"
#include "FileListPane.h"
#include "ViewerPanel.h"
#include "SettingsDialog.h"
#include "../keybinding/ICommand.h"
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QKeyEvent>
#include <QCloseEvent>
#include <QTableView>
#include <QApplication>
#include <QMessageBox>

namespace Farman {

MainWindow::MainWindow(QWidget* parent)
  : QMainWindow(parent)
  , m_stack(nullptr)
  , m_fileManagerPanel(nullptr)
  , m_viewerPanel(nullptr) {

  // Load settings first (before setupUi to apply window size/position)
  Settings::instance().load();

  setupUi();

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

  // Apply window size settings
  auto& settings = Settings::instance();
  QSize windowSize(1200, 600);  // Default size

  switch (settings.windowSizeMode()) {
    case WindowSizeMode::Default:
      windowSize = QSize(1200, 600);
      break;
    case WindowSizeMode::LastSession:
      windowSize = settings.lastWindowSize();
      break;
    case WindowSizeMode::Custom:
      windowSize = settings.customWindowSize();
      break;
  }

  resize(windowSize);

  // Apply window position settings
  switch (settings.windowPositionMode()) {
    case WindowPositionMode::Default:
      // Center window on screen (Qt default behavior)
      break;
    case WindowPositionMode::LastSession:
      move(settings.lastWindowPosition());
      break;
    case WindowPositionMode::Custom:
      move(settings.customWindowPosition());
      break;
  }

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
    "Up",
    [this]() {
      QKeyEvent event(QEvent::KeyPress, Qt::Key_Up, Qt::NoModifier);
      m_fileManagerPanel->handleKeyEvent(&event);
    },
    "navigation"
  ));

  registry.registerCommand(std::make_shared<LambdaCommand>(
    "navigate.down",
    "Down",
    [this]() {
      QKeyEvent event(QEvent::KeyPress, Qt::Key_Down, Qt::NoModifier);
      m_fileManagerPanel->handleKeyEvent(&event);
    },
    "navigation"
  ));

  registry.registerCommand(std::make_shared<LambdaCommand>(
    "navigate.left",
    "Left",
    [this]() {
      QKeyEvent event(QEvent::KeyPress, Qt::Key_Left, Qt::NoModifier);
      m_fileManagerPanel->handleKeyEvent(&event);
    },
    "navigation"
  ));

  registry.registerCommand(std::make_shared<LambdaCommand>(
    "navigate.right",
    "Right",
    [this]() {
      QKeyEvent event(QEvent::KeyPress, Qt::Key_Right, Qt::NoModifier);
      m_fileManagerPanel->handleKeyEvent(&event);
    },
    "navigation"
  ));

  registry.registerCommand(std::make_shared<LambdaCommand>(
    "navigate.home",
    "Jump to Top",
    [this]() {
      QKeyEvent event(QEvent::KeyPress, Qt::Key_Home, Qt::NoModifier);
      m_fileManagerPanel->handleKeyEvent(&event);
    },
    "navigation"
  ));

  registry.registerCommand(std::make_shared<LambdaCommand>(
    "navigate.end",
    "Jump to Bottom",
    [this]() {
      QKeyEvent event(QEvent::KeyPress, Qt::Key_End, Qt::NoModifier);
      m_fileManagerPanel->handleKeyEvent(&event);
    },
    "navigation"
  ));

  registry.registerCommand(std::make_shared<LambdaCommand>(
    "navigate.pageup",
    "Page Up",
    [this]() {
      QKeyEvent event(QEvent::KeyPress, Qt::Key_PageUp, Qt::NoModifier);
      m_fileManagerPanel->handleKeyEvent(&event);
    },
    "navigation"
  ));

  registry.registerCommand(std::make_shared<LambdaCommand>(
    "navigate.pagedown",
    "Page Down",
    [this]() {
      QKeyEvent event(QEvent::KeyPress, Qt::Key_PageDown, Qt::NoModifier);
      m_fileManagerPanel->handleKeyEvent(&event);
    },
    "navigation"
  ));

  registry.registerCommand(std::make_shared<LambdaCommand>(
    "navigate.enter",
    "Enter",
    [this]() {
      QKeyEvent event(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
      m_fileManagerPanel->handleKeyEvent(&event);
    },
    "navigation"
  ));

  registry.registerCommand(std::make_shared<LambdaCommand>(
    "navigate.parent",
    "Parent Directory",
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

  registry.registerCommand(std::make_shared<LambdaCommand>(
    "pane.sort_filter",
    "Sort & Filter...",
    [this]() {
      m_fileManagerPanel->openSortFilterDialog();
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
  // Apply the new settings to the file manager panel
  m_fileManagerPanel->applySettings();
}

void MainWindow::closeEvent(QCloseEvent* event) {
  auto& settings = Settings::instance();

  // Show confirmation dialog if enabled
  if (settings.confirmOnExit()) {
    QMessageBox::StandardButton reply = QMessageBox::question(
      this,
      tr("Confirm Exit"),
      tr("Are you sure you want to exit farman?"),
      QMessageBox::Yes | QMessageBox::No,
      QMessageBox::No
    );

    if (reply != QMessageBox::Yes) {
      event->ignore();
      return;
    }
  }

  // Save last window size and position
  settings.setLastWindowSize(size());
  settings.setLastWindowPosition(pos());
  settings.save();

  event->accept();
  QMainWindow::closeEvent(event);
}

} // namespace Farman
