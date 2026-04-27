#include "MainWindow.h"
#include "FileManagerPanel.h"
#include "FileListPane.h"
#include "ViewerPanel.h"
#include "SettingsDialog.h"
#include "BookmarkListDialog.h"
#include "HistoryDialog.h"
#include "SearchDialog.h"
#include "../core/Logger.h"
#include "../keybinding/ICommand.h"
#include "../core/DirectoryHistory.h"
#include "../utils/Dialogs.h"
#include <QStackedWidget>
#include <QStatusBar>
#include <QLabel>
#include <QFontMetrics>
#include <QVBoxLayout>
#include <QKeyEvent>
#include <QCloseEvent>
#include <QTableView>
#include <QApplication>
#include <QMessageBox>
#include <QMenuBar>
#include <QMenu>
#include <QAction>

namespace Farman {

MainWindow::MainWindow(QWidget* parent)
  : QMainWindow(parent)
  , m_stack(nullptr)
  , m_fileManagerPanel(nullptr)
  , m_viewerPanel(nullptr) {

  // Load settings first (before setupUi to apply window size/position)
  Settings::instance().load();

  // ロガーを設定 (ファイル出力 ON/OFF・出力先) し、起動の旨を 1 行残す
  {
    auto& s = Settings::instance();
    Logger::instance().setFileOutput(s.logToFile(), s.logFilePath());
    Logger::instance().info(QStringLiteral("farman started"));
  }

  setupUi();

  // Register commands
  registerCommands();

  // Load keybindings
  KeyBindingManager::instance().loadFromSettings();

  // メニューバーはキーバインドが確定した後に作る（右端にショートカットを表示するため）
  createMenus();

  // 履歴は永続化が ON の場合のみ、初期パス読み込みの前に復元しておく。
  // loadInitialPath() が navigatePane() を呼び、現在パスが履歴の先頭に自然に入る。
  if (Settings::instance().persistHistory()) {
    m_fileManagerPanel->history(PaneType::Left).setEntries(
      Settings::instance().paneHistory(PaneType::Left));
    m_fileManagerPanel->history(PaneType::Right).setEntries(
      Settings::instance().paneHistory(PaneType::Right));
  }

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

  // ===== Status Bar =====
  m_statusPathLabel = new QLabel(this);
  m_statusPathLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
  // 長いパスは末尾を表示できるよう中央付近を省略 (... で省略表示)
  m_statusPathLabel->setMinimumWidth(0);
  m_statusPathLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  m_statusSummaryLabel = new QLabel(this);
  m_statusSummaryLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
  statusBar()->addWidget(m_statusPathLabel, /*stretch*/ 1);
  statusBar()->addPermanentWidget(m_statusSummaryLabel);

  connect(m_fileManagerPanel, &FileManagerPanel::activeFocusedPathChanged,
          this, [this](const QString& path) {
    m_fmStatusPath = path;
    if (m_stack->currentWidget() == m_fileManagerPanel) updateStatusBar();
  });
  connect(m_fileManagerPanel, &FileManagerPanel::activeSummaryChanged,
          this, [this](const QString& summary) {
    m_fmStatusSummary = summary;
    if (m_stack->currentWidget() == m_fileManagerPanel) updateStatusBar();
  });
  connect(m_viewerPanel, &ViewerPanel::viewerStatusChanged,
          this, [this](const QString& path, const QString& summary) {
    m_viewerStatusPath = path;
    m_viewerStatusSummary = summary;
    if (m_stack->currentWidget() == m_viewerPanel) updateStatusBar();
  });
}

void MainWindow::updateStatusBar() {
  const bool fm = (m_stack && m_stack->currentWidget() == m_fileManagerPanel);
  const QString& path    = fm ? m_fmStatusPath    : m_viewerStatusPath;
  const QString& summary = fm ? m_fmStatusSummary : m_viewerStatusSummary;
  m_statusPathLabel->setText(path);
  m_statusPathLabel->setToolTip(path);
  m_statusSummaryLabel->setText(summary);
}

void MainWindow::showFileManager() {
  if (m_stack->currentWidget() != m_fileManagerPanel) {
    m_stack->setCurrentWidget(m_fileManagerPanel);
    m_viewerPanel->clear();

    // フォーカスをアクティブペインに戻す
    m_fileManagerPanel->activePane()->view()->setFocus();
    updateStatusBar();
  }
}

void MainWindow::showViewer(const QString& filePath) {
  if (m_viewerPanel->openFile(filePath)) {
    m_stack->setCurrentWidget(m_viewerPanel);
    m_viewerPanel->setFocus();
    updateStatusBar();
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
      // カーソル据え置きで選択トグル。
      m_fileManagerPanel->toggleSelection();
    },
    "selection"
  ));

  registry.registerCommand(std::make_shared<LambdaCommand>(
    "select.toggle_and_down",
    "Toggle Selection and Move Down",
    [this]() {
      // 選択トグル後にカーソルを下へ。handleSpaceKey の挙動。
      QKeyEvent event(QEvent::KeyPress, Qt::Key_Space, Qt::NoModifier);
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

  registry.registerCommand(std::make_shared<LambdaCommand>(
    "file.newfile",
    "New File",
    [this]() {
      m_fileManagerPanel->createFile();
    },
    "file"
  ));

  registry.registerCommand(std::make_shared<LambdaCommand>(
    "file.attributes",
    "Change Attributes",
    [this]() {
      m_fileManagerPanel->changeAttributes();
    },
    "file"
  ));

  registry.registerCommand(std::make_shared<LambdaCommand>(
    "file.search",
    "Search Files...",
    [this]() {
      const QString start = m_fileManagerPanel->activePane()->currentPath();
      SearchDialog dlg(start, this);
      if (dlg.exec() == QDialog::Accepted) {
        const QString picked = dlg.selectedPath();
        if (!picked.isEmpty()) {
          m_fileManagerPanel->navigateActivePaneTo(picked);
        }
      }
      m_fileManagerPanel->activePane()->view()->setFocus();
    },
    "file"
  ));

  registry.registerCommand(std::make_shared<LambdaCommand>(
    "file.pack",
    "Create Archive",
    [this]() { m_fileManagerPanel->createArchive(); },
    "file"
  ));

  registry.registerCommand(std::make_shared<LambdaCommand>(
    "file.unpack",
    "Extract Archive",
    [this]() { m_fileManagerPanel->extractArchive(); },
    "file"
  ));

  registry.registerCommand(std::make_shared<LambdaCommand>(
    "file.rename",
    "Rename",
    [this]() {
      m_fileManagerPanel->renameItem();
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

  registry.registerCommand(std::make_shared<LambdaCommand>(
    "view.toggle_log",
    "Toggle Log Pane",
    [this]() {
      const bool nowVisible = !m_fileManagerPanel->isLogPaneVisible();
      m_fileManagerPanel->setLogPaneVisible(nowVisible);
      Settings::instance().setLogVisible(nowVisible);
      Settings::instance().save();
    },
    "view"
  ));

  // Bookmark commands
  registry.registerCommand(std::make_shared<LambdaCommand>(
    "bookmark.toggle",
    "Toggle Bookmark",
    [this]() {
      m_fileManagerPanel->activePane()->toggleBookmarkForCurrentPath();
    },
    "bookmark"
  ));

  registry.registerCommand(std::make_shared<LambdaCommand>(
    "bookmark.list",
    "Bookmarks...",
    [this]() {
      BookmarkListDialog dlg(this);
      if (dlg.exec() == QDialog::Accepted) {
        const QString path = dlg.selectedPath();
        if (!path.isEmpty()) {
          m_fileManagerPanel->navigateActivePaneTo(path);
        }
      }
      // ダイアログ閉じた後もアクティブペインにフォーカスを戻す
      m_fileManagerPanel->activePane()->view()->setFocus();
    },
    "bookmark"
  ));

  // History commands
  registry.registerCommand(std::make_shared<LambdaCommand>(
    "history.show",
    "History...",
    [this]() {
      const DirectoryHistory& hist = m_fileManagerPanel->history(
        m_fileManagerPanel->activePane() == m_fileManagerPanel->leftPane()
          ? PaneType::Left : PaneType::Right);
      HistoryDialog dlg(hist.entries(), this);
      if (dlg.exec() == QDialog::Accepted) {
        const QString path = dlg.selectedPath();
        if (!path.isEmpty()) {
          m_fileManagerPanel->navigateActivePaneTo(path);
        }
      }
      m_fileManagerPanel->activePane()->view()->setFocus();
    },
    "history"
  ));
}

void MainWindow::createMenus() {
  QMenuBar* bar = menuBar();

  // コマンドIDとラベルから QAction を作る。現在のキーバインドをショートカットとして
  // 設定し、メニュー右側に表示する。triggered は CommandRegistry 経由で実行するので
  // キー操作とメニュークリックで同じパスを通る。
  //
  // global=false のショートカットは FileManagerPanel スコープに限定する。
  // c / m / d のような 1 文字キーがビュアー表示中に意図せず発火するのを防ぐため。
  // macOS のネイティブメニューでは Insert キーが特殊な Unicode 字形（⎀, U+2380）
  // に変換されるが、Mac の標準フォントでは描画できず文字化けしたように見える。
  // 加えて Mac キーボードには Insert キーが無いので、メニュー上に表示する意味も薄い。
  // そういうキーが含まれる QKeySequence はメニューには出さない（eventFilter 経由の
  // キー操作は引き続き効くので機能は失われない）。
  auto isMenuDisplayable = [](const QKeySequence& seq) {
#ifdef Q_OS_MACOS
    for (int i = 0; i < seq.count(); ++i) {
      const Qt::Key k = seq[i].key();
      if (k == Qt::Key_Insert) return false;
    }
#else
    Q_UNUSED(seq);
#endif
    return true;
  };

  auto addCmd = [this, isMenuDisplayable](QMenu* menu, const QString& id, const QString& label,
                                          bool global = false) -> QAction* {
    QAction* action = new QAction(label, this);
    const QList<QKeySequence> keys = KeyBindingManager::instance().keysForCommand(id);
    if (!keys.isEmpty() && isMenuDisplayable(keys.first())) {
      action->setShortcut(keys.first());
      if (!global) {
        action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        m_fileManagerPanel->addAction(action);
      }
    }
    connect(action, &QAction::triggered, this, [id]() {
      CommandRegistry::instance().execute(id);
    });
    menu->addAction(action);
    return action;
  };

  // File
  QMenu* fileMenu = bar->addMenu(tr("&File"));
  addCmd(fileMenu, "file.newfile",    tr("New File"));
  addCmd(fileMenu, "file.mkdir",      tr("New Directory"));
  addCmd(fileMenu, "file.rename",     tr("Rename"));
  fileMenu->addSeparator();
  addCmd(fileMenu, "file.copy",       tr("Copy"));
  addCmd(fileMenu, "file.move",       tr("Move"));
  addCmd(fileMenu, "file.delete",     tr("Delete"));
  fileMenu->addSeparator();
  addCmd(fileMenu, "file.attributes", tr("Change Attributes..."));
  fileMenu->addSeparator();
  addCmd(fileMenu, "file.pack",       tr("Create Archive..."));
  addCmd(fileMenu, "file.unpack",     tr("Extract Archive..."));
  fileMenu->addSeparator();
  // Ctrl+Q はウィンドウ全体（ビュアー表示中でも）効くべきなので global=true
  QAction* quitAction = addCmd(fileMenu, "app.quit", tr("Quit"), /*global=*/true);
  // macOS ではアプリケーションメニューに自動で移動する
  quitAction->setMenuRole(QAction::QuitRole);

  // Edit（macOS では "Start Dictation" / "Auto Fill" 等のシステム項目が自動で
  // 追加されるが、慣例的な名前のほうがわかりやすいので Edit のまま）
  QMenu* editMenu = bar->addMenu(tr("&Edit"));
  addCmd(editMenu, "select.all",             tr("Select All"));
  addCmd(editMenu, "select.invert",          tr("Invert Selection"));
  addCmd(editMenu, "select.toggle",          tr("Toggle Selection"));
  addCmd(editMenu, "select.toggle_and_down", tr("Toggle and Move Down"));

  // View
  QMenu* viewMenu = bar->addMenu(tr("&View"));
  addCmd(viewMenu, "pane.switch", tr("Switch Pane"));
  // Single Pane はトグル状態をチェックマークで表示する。キー操作・メニュー操作・
  // 設定経由など複数の経路でモードが変わるため、メニューを開く直前に最新状態に
  // 同期する（aboutToShow）。
  QAction* singlePaneAction = addCmd(viewMenu, "pane.toggle_single", tr("Single Pane"));
  singlePaneAction->setCheckable(true);
  singlePaneAction->setChecked(m_fileManagerPanel->isSinglePaneMode());
  connect(viewMenu, &QMenu::aboutToShow, this, [this, singlePaneAction]() {
    singlePaneAction->setChecked(m_fileManagerPanel->isSinglePaneMode());
  });
  addCmd(viewMenu, "pane.sort_filter", tr("Sort && Filter..."));
  viewMenu->addSeparator();
  addCmd(viewMenu, "view.file", tr("View File"));
  addCmd(viewMenu, "view.toggle_log", tr("Toggle Log Pane"));

  // Go
  QMenu* goMenu = bar->addMenu(tr("&Go"));
  addCmd(goMenu, "navigate.parent", tr("Parent Directory"));
  addCmd(goMenu, "navigate.home",   tr("Jump to Top"));
  addCmd(goMenu, "navigate.end",    tr("Jump to Bottom"));
  goMenu->addSeparator();
  addCmd(goMenu, "file.search",     tr("Search Files..."));
  addCmd(goMenu, "history.show",    tr("History..."));

  // Bookmarks
  QMenu* bookmarksMenu = bar->addMenu(tr("&Bookmarks"));
  addCmd(bookmarksMenu, "bookmark.toggle", tr("Toggle Bookmark"));
  addCmd(bookmarksMenu, "bookmark.list",   tr("Bookmarks..."));

  // Help
  QMenu* helpMenu = bar->addMenu(tr("&Help"));
  // Settings はビュアー表示中でも開けるように global=true
  QAction* settingsAction = addCmd(helpMenu, "app.settings", tr("Settings..."), /*global=*/true);
  // macOS ではアプリケーションメニューの Preferences に移動する
  settingsAction->setMenuRole(QAction::PreferencesRole);
  QAction* aboutAction = new QAction(tr("About farman..."), this);
  aboutAction->setMenuRole(QAction::AboutRole);
  connect(aboutAction, &QAction::triggered, this, &MainWindow::showAboutDialog);
  helpMenu->addAction(aboutAction);
}

void MainWindow::showAboutDialog() {
  QMessageBox::about(this, tr("About farman"),
    tr("<b>farman</b> - File Manager<br><br>"
       "A keyboard-driven Qt6 file manager inspired by "
       "Total Commander / Double Commander."));
}

void MainWindow::showSettingsDialog() {
  SettingsDialog* dialog = new SettingsDialog(
    m_fileManagerPanel->leftPath(),
    m_fileManagerPanel->rightPath(),
    this
  );
  connect(dialog, &SettingsDialog::settingsChanged, this, &MainWindow::onSettingsChanged);
  dialog->exec();
  delete dialog;
}

void MainWindow::onSettingsChanged() {
  // Settings have been changed and saved
  // Apply the new settings to the file manager panel
  m_fileManagerPanel->applySettings();

  // ログ表示・ファイル出力の状態を Settings に追従
  auto& s = Settings::instance();
  m_fileManagerPanel->setLogPaneVisible(s.logVisible());
  Logger::instance().setFileOutput(s.logToFile(), s.logFilePath());
}

void MainWindow::closeEvent(QCloseEvent* event) {
  auto& settings = Settings::instance();

  // Show confirmation dialog if enabled
  if (settings.confirmOnExit()) {
    if (!confirm(this, tr("Confirm Exit"),
                 tr("Are you sure you want to exit farman?"))) {
      event->ignore();
      return;
    }
  }

  // Save last window size and position
  settings.setLastWindowSize(size());
  settings.setLastWindowPosition(pos());

  // Persist each pane's current path so InitialPathMode::LastSession works.
  auto storeLastPath = [&settings](PaneType type, FileListPane* pane) {
    PaneSettings s = settings.paneSettings(type);
    s.path = pane->currentPath();
    settings.setPaneSettings(type, s);
  };
  storeLastPath(PaneType::Left,  m_fileManagerPanel->leftPane());
  storeLastPath(PaneType::Right, m_fileManagerPanel->rightPane());

  // 履歴の永続化: ON のときは現在の履歴を保存、OFF のときは空で上書きして残留を消す
  auto storeHistory = [&settings, this](PaneType type) {
    if (settings.persistHistory()) {
      settings.setPaneHistory(type, m_fileManagerPanel->history(type).entries());
    } else {
      settings.setPaneHistory(type, {});
    }
  };
  storeHistory(PaneType::Left);
  storeHistory(PaneType::Right);

  settings.save();

  event->accept();
  QMainWindow::closeEvent(event);
}

} // namespace Farman
