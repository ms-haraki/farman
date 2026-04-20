#include "FileManagerPanel.h"
#include "FileListPane.h"
#include "ProgressDialog.h"
#include "SortFilterDialog.h"
#include "model/FileListModel.h"
#include "core/FileItem.h"
#include "core/workers/CopyWorker.h"
#include "core/workers/MoveWorker.h"
#include "core/workers/RemoveWorker.h"
#include "settings/Settings.h"
#include <QVBoxLayout>
#include <QSplitter>
#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>
#include <QDir>
#include <QKeyEvent>
#include <QTableView>
#include <QHeaderView>
#include <QFile>

namespace Farman {

FileManagerPanel::FileManagerPanel(QWidget* parent)
  : QWidget(parent)
  , m_splitter(nullptr)
  , m_leftPane(nullptr)
  , m_rightPane(nullptr)
  , m_activePane(PaneType::Left)
  , m_singlePaneMode(false) {

  setupUi();
}

FileManagerPanel::~FileManagerPanel() = default;

void FileManagerPanel::setupUi() {
  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);

  // Splitter for two panes
  m_splitter = new QSplitter(Qt::Horizontal, this);
  layout->addWidget(m_splitter);

  // ===== Left Pane =====
  m_leftPane = new FileListPane(this);
  connect(m_leftPane, &FileListPane::currentChanged, this, &FileManagerPanel::onLeftPaneCurrentChanged);
  connect(m_leftPane, &FileListPane::folderButtonClicked, this, &FileManagerPanel::onLeftFolderButtonClicked);
  m_splitter->addWidget(m_leftPane);

  // ===== Right Pane =====
  m_rightPane = new FileListPane(this);
  connect(m_rightPane, &FileListPane::currentChanged, this, &FileManagerPanel::onRightPaneCurrentChanged);
  connect(m_rightPane, &FileListPane::folderButtonClicked, this, &FileManagerPanel::onRightFolderButtonClicked);
  m_splitter->addWidget(m_rightPane);

  // Splitterのサイズを均等に
  m_splitter->setSizes(QList<int>() << 600 << 600);
}

void FileManagerPanel::loadInitialPath() {
  auto& settings = Settings::instance();

  PaneSettings leftSettings = settings.paneSettings(PaneType::Left);
  PaneSettings rightSettings = settings.paneSettings(PaneType::Right);

  QString leftPath = settings.restoreLastPath() ? leftSettings.path : QDir::homePath();
  QString rightPath = settings.restoreLastPath() ? rightSettings.path : QDir::homePath();

  navigatePane(PaneType::Left, leftPath);
  navigatePane(PaneType::Right, rightPath);

  // 左ペインをアクティブに
  setActivePane(PaneType::Left);

  updatePathSignal();
}

void FileManagerPanel::applySettings() {
  // Behavior タブでデフォルト設定が変更された際の再適用。
  // パス単位の override があればそちらを優先する。
  applyPathSortFilter(PaneType::Left);
  applyPathSortFilter(PaneType::Right);

  m_leftPane->model()->refresh();
  m_rightPane->model()->refresh();
}

void FileManagerPanel::applyPathSortFilter(PaneType paneType) {
  FileListPane* pane = (paneType == PaneType::Left) ? m_leftPane : m_rightPane;
  FileListModel* model = pane->model();
  auto& settings = Settings::instance();
  const QString path = pane->currentPath();

  PaneSettings s = settings.hasPathOverride(path)
                   ? settings.pathOverride(path)
                   : settings.paneSettings(paneType);

  model->setSortSettings(
    s.sortKey, s.sortOrder, s.sortKey2nd,
    s.sortDirsType, s.sortDotFirst, s.sortCS
  );
  model->setAttrFilter(s.attrFilter);
  model->setNameFilters(s.nameFilters);

  // Header sort indicator を現在のソート状態に合わせる
  int section = -1;
  switch (s.sortKey) {
    case SortKey::Name:         section = FileListModel::Name;         break;
    case SortKey::Size:         section = FileListModel::Size;         break;
    case SortKey::Type:         section = FileListModel::Type;         break;
    case SortKey::LastModified: section = FileListModel::LastModified; break;
    case SortKey::None: break;
  }
  if (section >= 0) {
    pane->view()->horizontalHeader()->setSortIndicator(section, s.sortOrder);
  }

  pane->refreshSortFilterStatus();
}

bool FileManagerPanel::navigatePane(PaneType paneType, const QString& path) {
  FileListPane* pane = (paneType == PaneType::Left) ? m_leftPane : m_rightPane;
  FileListModel* model = pane->model();
  auto& settings = Settings::instance();

  PaneSettings s = settings.hasPathOverride(path)
                   ? settings.pathOverride(path)
                   : settings.paneSettings(paneType);

  model->setSortSettings(
    s.sortKey, s.sortOrder, s.sortKey2nd,
    s.sortDirsType, s.sortDotFirst, s.sortCS
  );
  model->setAttrFilter(s.attrFilter);
  model->setNameFilters(s.nameFilters);

  return pane->setPath(path);
}

bool FileManagerPanel::handleKeyEvent(QKeyEvent* event) {
  // Ctrl+U (Windows/Linux) or Cmd+U (Mac) で1ペイン/2ペイン切り替え
  if ((event->modifiers() & Qt::ControlModifier || event->modifiers() & Qt::MetaModifier)
      && event->key() == Qt::Key_U) {
    togglePaneMode();
    return true;
  }

  // Tabキーでペイン切り替え（2ペイン表示時のみ）
  if (event->key() == Qt::Key_Tab && !m_singlePaneMode) {
    handleTabKey();
    return true;
  }

  // 左キーの処理
  if (event->key() == Qt::Key_Left) {
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
  if (event->key() == Qt::Key_Right) {
    if (m_singlePaneMode) {
      // シングルペインモード: 何もしない
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
  if ((event->modifiers() & Qt::ControlModifier || event->modifiers() & Qt::MetaModifier)
      && event->key() == Qt::Key_A) {
    handleSelectAllKey();
    return true;
  }

  FileListPane* pane = activePane();
  FileListModel* model = pane->model();

  switch (event->key()) {
    case Qt::Key_Up: {
      QModelIndex current = pane->view()->currentIndex();
      if (!current.isValid()) {
        return true;
      }
      int rows = model->rowCount();
      if (current.row() > 0) {
        pane->view()->setCurrentIndex(model->index(current.row() - 1, 0));
      } else if (Settings::instance().cursorLoop() && rows > 0) {
        pane->view()->setCurrentIndex(model->index(rows - 1, 0));
      }
      return true;
    }

    case Qt::Key_Down: {
      QModelIndex current = pane->view()->currentIndex();
      if (!current.isValid()) {
        return true;
      }
      int rows = model->rowCount();
      if (current.row() < rows - 1) {
        pane->view()->setCurrentIndex(model->index(current.row() + 1, 0));
      } else if (Settings::instance().cursorLoop() && rows > 0) {
        pane->view()->setCurrentIndex(model->index(0, 0));
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

    case Qt::Key_F2:
      renameItem();
      return true;

    case Qt::Key_F5:
      copySelectedFiles();
      return true;

    case Qt::Key_F6:
      moveSelectedFiles();
      return true;

    case Qt::Key_F7:
      createDirectory();
      return true;

    case Qt::Key_F8:
    case Qt::Key_Delete:
      deleteSelectedFiles();
      return true;

    default:
      break;
  }

  return false;
}

void FileManagerPanel::onLeftPaneCurrentChanged(const QModelIndex& current, const QModelIndex& previous) {
  Q_UNUSED(current);
  Q_UNUSED(previous);
  m_leftPane->view()->viewport()->update();
}

void FileManagerPanel::onRightPaneCurrentChanged(const QModelIndex& current, const QModelIndex& previous) {
  Q_UNUSED(current);
  Q_UNUSED(previous);
  m_rightPane->view()->viewport()->update();
}

void FileManagerPanel::handleEnterKey() {
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
    if (navigatePane(m_activePane, newPath)) {
      updatePathSignal();
    }
  } else {
    // ファイルの場合はシグナルを発行
    emit fileActivated(item->absolutePath());
  }
}

void FileManagerPanel::handleBackspaceKey() {
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
  if (navigatePane(m_activePane, parentPath)) {
    updatePathSignal();
  }
}

void FileManagerPanel::handleSpaceKey() {
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

void FileManagerPanel::handleInsertKey() {
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

void FileManagerPanel::handleAsteriskKey() {
  FileListModel* model = activePane()->model();
  // 選択を反転
  model->invertSelection();
}

void FileManagerPanel::handleSelectAllKey() {
  FileListModel* model = activePane()->model();
  // 全て選択されている場合は全解除、それ以外は全選択
  if (model->isAllSelected()) {
    model->setSelectedAll(false);
  } else {
    model->setSelectedAll(true);
  }
}

void FileManagerPanel::handleTabKey() {
  // アクティブペインを切り替え（2ペイン表示時のみ）
  if (!m_singlePaneMode) {
    if (m_activePane == PaneType::Left) {
      setActivePane(PaneType::Right);
    } else {
      setActivePane(PaneType::Left);
    }
  }
}

void FileManagerPanel::togglePaneMode() {
  setSinglePaneMode(!m_singlePaneMode);
}

void FileManagerPanel::setSinglePaneMode(bool single) {
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

FileListPane* FileManagerPanel::activePane() const {
  return (m_activePane == PaneType::Left) ? m_leftPane : m_rightPane;
}

void FileManagerPanel::setActivePane(PaneType pane) {
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

void FileManagerPanel::onLeftFolderButtonClicked() {
  QString currentPath = m_leftPane->currentPath();
  QString selectedPath = QFileDialog::getExistingDirectory(
    this,
    tr("Select Folder"),
    currentPath,
    QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
  );

  if (!selectedPath.isEmpty() && selectedPath != currentPath) {
    if (navigatePane(PaneType::Left, selectedPath)) {
      updatePathSignal();
    }
  }
}

void FileManagerPanel::onRightFolderButtonClicked() {
  QString currentPath = m_rightPane->currentPath();
  QString selectedPath = QFileDialog::getExistingDirectory(
    this,
    tr("Select Folder"),
    currentPath,
    QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
  );

  if (!selectedPath.isEmpty() && selectedPath != currentPath) {
    if (navigatePane(PaneType::Right, selectedPath)) {
      updatePathSignal();
    }
  }
}

QString FileManagerPanel::currentPath() const {
  return activePane()->currentPath();
}

QString FileManagerPanel::leftPath() const {
  return m_leftPane->currentPath();
}

QString FileManagerPanel::rightPath() const {
  return m_rightPane->currentPath();
}

void FileManagerPanel::updatePathSignal() {
  emit pathChanged(m_leftPane->currentPath(), m_rightPane->currentPath());
}

void FileManagerPanel::copySelectedFiles() {
  FileListPane* srcPane = activePane();
  FileListPane* destPane = (m_activePane == PaneType::Left) ? m_rightPane : m_leftPane;

  FileListModel* srcModel = srcPane->model();
  QString destDir = destPane->currentPath();

  // Get selected files, or current item if no selection
  QStringList selectedFiles = srcModel->selectedFilePaths();
  if (selectedFiles.isEmpty()) {
    QModelIndex currentIndex = srcPane->view()->currentIndex();
    if (currentIndex.isValid()) {
      const FileItem* item = srcModel->itemAt(currentIndex);
      if (item && !item->isDotDot()) {
        selectedFiles.append(item->absolutePath());
      }
    }
  }

  if (selectedFiles.isEmpty()) {
    return;
  }

  // Create worker and dialog
  CopyWorker* worker = new CopyWorker(selectedFiles, destDir, this);
  ProgressDialog* dialog = new ProgressDialog(tr("Copying files..."), this);
  dialog->setWorker(worker);

  connect(worker, &WorkerBase::finished, this, [this, dialog, srcPane, destPane](bool success) {
    dialog->accept();

    // Save cursor positions
    QModelIndex srcCurrentIndex = srcPane->view()->currentIndex();
    QModelIndex destCurrentIndex = destPane->view()->currentIndex();
    QString srcCurrentFile;
    QString destCurrentFile;
    int srcCurrentRow = srcCurrentIndex.row();
    int destCurrentRow = destCurrentIndex.row();

    if (srcCurrentIndex.isValid()) {
      const FileItem* item = srcPane->model()->itemAt(srcCurrentIndex);
      if (item) srcCurrentFile = item->name();
    }
    if (destCurrentIndex.isValid()) {
      const FileItem* item = destPane->model()->itemAt(destCurrentIndex);
      if (item) destCurrentFile = item->name();
    }

    // Refresh both panes
    QString srcPath = srcPane->currentPath();
    QString destPath = destPane->currentPath();
    srcPane->setPath(srcPath);
    destPane->setPath(destPath);

    // Restore cursor positions
    auto restoreCursor = [](FileListPane* pane, const QString& fileName, int fallbackRow) {
      FileListModel* model = pane->model();

      // Try to find the file by name
      if (!fileName.isEmpty()) {
        for (int i = 0; i < model->rowCount(); ++i) {
          const FileItem* item = model->itemAt(i);
          if (item && item->name() == fileName) {
            pane->view()->setCurrentIndex(model->index(i, 0));
            return;
          }
        }
      }

      // Fallback to row number (clamped to valid range)
      if (model->rowCount() > 0) {
        int row = qMin(fallbackRow, model->rowCount() - 1);
        pane->view()->setCurrentIndex(model->index(row, 0));
      }
    };

    restoreCursor(srcPane, srcCurrentFile, srcCurrentRow);
    restoreCursor(destPane, destCurrentFile, destCurrentRow);

    // Clear selections
    srcPane->model()->setSelectedAll(false);

    // Restore focus to active pane
    activePane()->view()->setFocus();
  });

  worker->start();
  dialog->exec();
}

void FileManagerPanel::moveSelectedFiles() {
  FileListPane* srcPane = activePane();
  FileListPane* destPane = (m_activePane == PaneType::Left) ? m_rightPane : m_leftPane;

  FileListModel* srcModel = srcPane->model();
  QString destDir = destPane->currentPath();

  // Get selected files, or current item if no selection
  QStringList selectedFiles = srcModel->selectedFilePaths();
  if (selectedFiles.isEmpty()) {
    QModelIndex currentIndex = srcPane->view()->currentIndex();
    if (currentIndex.isValid()) {
      const FileItem* item = srcModel->itemAt(currentIndex);
      if (item && !item->isDotDot()) {
        selectedFiles.append(item->absolutePath());
      }
    }
  }

  if (selectedFiles.isEmpty()) {
    return;
  }

  // Create worker and dialog
  MoveWorker* worker = new MoveWorker(selectedFiles, destDir, this);
  ProgressDialog* dialog = new ProgressDialog(tr("Moving files..."), this);
  dialog->setWorker(worker);

  connect(worker, &WorkerBase::finished, this, [this, dialog, srcPane, destPane](bool success) {
    dialog->accept();

    // Save cursor positions
    QModelIndex srcCurrentIndex = srcPane->view()->currentIndex();
    QModelIndex destCurrentIndex = destPane->view()->currentIndex();
    QString srcCurrentFile;
    QString destCurrentFile;
    int srcCurrentRow = srcCurrentIndex.row();
    int destCurrentRow = destCurrentIndex.row();

    if (srcCurrentIndex.isValid()) {
      const FileItem* item = srcPane->model()->itemAt(srcCurrentIndex);
      if (item) srcCurrentFile = item->name();
    }
    if (destCurrentIndex.isValid()) {
      const FileItem* item = destPane->model()->itemAt(destCurrentIndex);
      if (item) destCurrentFile = item->name();
    }

    // Refresh both panes
    QString srcPath = srcPane->currentPath();
    QString destPath = destPane->currentPath();
    srcPane->setPath(srcPath);
    destPane->setPath(destPath);

    // Restore cursor positions
    auto restoreCursor = [](FileListPane* pane, const QString& fileName, int fallbackRow) {
      FileListModel* model = pane->model();

      // Try to find the file by name
      if (!fileName.isEmpty()) {
        for (int i = 0; i < model->rowCount(); ++i) {
          const FileItem* item = model->itemAt(i);
          if (item && item->name() == fileName) {
            pane->view()->setCurrentIndex(model->index(i, 0));
            return;
          }
        }
      }

      // Fallback to row number (clamped to valid range)
      if (model->rowCount() > 0) {
        int row = qMin(fallbackRow, model->rowCount() - 1);
        pane->view()->setCurrentIndex(model->index(row, 0));
      }
    };

    restoreCursor(srcPane, srcCurrentFile, srcCurrentRow);
    restoreCursor(destPane, destCurrentFile, destCurrentRow);

    // Clear selections
    srcPane->model()->setSelectedAll(false);

    // Restore focus to active pane
    activePane()->view()->setFocus();
  });

  worker->start();
  dialog->exec();
}

void FileManagerPanel::deleteSelectedFiles() {
  FileListPane* srcPane = activePane();
  FileListModel* srcModel = srcPane->model();

  // Get selected files, or current item if no selection
  QStringList selectedFiles = srcModel->selectedFilePaths();
  if (selectedFiles.isEmpty()) {
    QModelIndex currentIndex = srcPane->view()->currentIndex();
    if (currentIndex.isValid()) {
      const FileItem* item = srcModel->itemAt(currentIndex);
      if (item && !item->isDotDot()) {
        selectedFiles.append(item->absolutePath());
      }
    }
  }

  if (selectedFiles.isEmpty()) {
    return;
  }

  // Ask for confirmation
  QString message;
  if (selectedFiles.size() == 1) {
    QFileInfo info(selectedFiles[0]);
    message = tr("Are you sure you want to delete '%1'?").arg(info.fileName());
  } else {
    message = tr("Are you sure you want to delete %1 items?").arg(selectedFiles.size());
  }

  QMessageBox::StandardButton reply = QMessageBox::question(
    this,
    tr("Confirm Delete"),
    message,
    QMessageBox::Yes | QMessageBox::No,
    QMessageBox::No
  );

  if (reply != QMessageBox::Yes) {
    return;
  }

  // Create worker and dialog
  RemoveWorker* worker = new RemoveWorker(selectedFiles, true, this);  // true = to trash
  ProgressDialog* dialog = new ProgressDialog(tr("Deleting files..."), this);
  dialog->setWorker(worker);

  connect(worker, &WorkerBase::finished, this, [this, dialog, srcPane](bool success) {
    dialog->accept();

    // Save cursor position
    QModelIndex srcCurrentIndex = srcPane->view()->currentIndex();
    QString srcCurrentFile;
    int srcCurrentRow = srcCurrentIndex.row();

    if (srcCurrentIndex.isValid()) {
      const FileItem* item = srcPane->model()->itemAt(srcCurrentIndex);
      if (item) srcCurrentFile = item->name();
    }

    // Refresh the pane
    QString srcPath = srcPane->currentPath();
    srcPane->setPath(srcPath);

    // Restore cursor position
    auto restoreCursor = [](FileListPane* pane, const QString& fileName, int fallbackRow) {
      FileListModel* model = pane->model();

      // Try to find the file by name
      if (!fileName.isEmpty()) {
        for (int i = 0; i < model->rowCount(); ++i) {
          const FileItem* item = model->itemAt(i);
          if (item && item->name() == fileName) {
            pane->view()->setCurrentIndex(model->index(i, 0));
            return;
          }
        }
      }

      // Fallback to row number (clamped to valid range)
      if (model->rowCount() > 0) {
        int row = qMin(fallbackRow, model->rowCount() - 1);
        pane->view()->setCurrentIndex(model->index(row, 0));
      }
    };

    restoreCursor(srcPane, srcCurrentFile, srcCurrentRow);

    // Clear selections
    srcPane->model()->setSelectedAll(false);

    // Restore focus to active pane
    activePane()->view()->setFocus();
  });

  worker->start();
  dialog->exec();
}

void FileManagerPanel::createDirectory() {
  FileListPane* srcPane = activePane();
  QString currentPath = srcPane->currentPath();

  // Ask for directory name
  bool ok;
  QString dirName = QInputDialog::getText(
    this,
    tr("Create Directory"),
    tr("Enter directory name:"),
    QLineEdit::Normal,
    QString(),
    &ok
  );

  if (!ok || dirName.isEmpty()) {
    return;
  }

  // Create directory
  QString newDirPath = currentPath + "/" + dirName;
  QDir dir;
  if (!dir.mkpath(newDirPath)) {
    QMessageBox::critical(
      this,
      tr("Error"),
      tr("Failed to create directory: %1").arg(dirName)
    );
    return;
  }

  // Refresh and move cursor to new directory
  srcPane->setPath(currentPath);

  // Find and select the new directory
  FileListModel* model = srcPane->model();
  for (int i = 0; i < model->rowCount(); ++i) {
    const FileItem* item = model->itemAt(i);
    if (item && item->name() == dirName) {
      srcPane->view()->setCurrentIndex(model->index(i, 0));
      break;
    }
  }

  srcPane->view()->setFocus();
}

void FileManagerPanel::openSortFilterDialog() {
  FileListPane* pane = activePane();
  const QString path = pane->currentPath();
  if (path.isEmpty()) {
    return;
  }

  auto& settings = Settings::instance();
  const bool alreadySaved = settings.hasPathOverride(path);

  // 初期値は override があればそれ、なければペインの既定
  PaneSettings initial = alreadySaved
                         ? settings.pathOverride(path)
                         : settings.paneSettings(m_activePane);

  SortFilterDialog dialog(path, initial, alreadySaved, this);
  if (dialog.exec() != QDialog::Accepted) {
    return;
  }

  PaneSettings result = dialog.result();

  // 保存 or 削除
  if (dialog.saveForDirectory()) {
    settings.setPathOverride(path, result);
    settings.save();
  } else if (alreadySaved) {
    settings.removePathOverride(path);
    settings.save();
  }

  // 現在のビューに即座に適用
  FileListModel* model = pane->model();
  model->setSortSettings(
    result.sortKey, result.sortOrder, result.sortKey2nd,
    result.sortDirsType, result.sortDotFirst, result.sortCS
  );
  model->setAttrFilter(result.attrFilter);
  model->setNameFilters(result.nameFilters);

  int section = -1;
  switch (result.sortKey) {
    case SortKey::Name:         section = FileListModel::Name;         break;
    case SortKey::Size:         section = FileListModel::Size;         break;
    case SortKey::Type:         section = FileListModel::Type;         break;
    case SortKey::LastModified: section = FileListModel::LastModified; break;
    case SortKey::None: break;
  }
  if (section >= 0) {
    pane->view()->horizontalHeader()->setSortIndicator(section, result.sortOrder);
  }

  pane->refreshSortFilterStatus();
  pane->view()->setFocus();
}

void FileManagerPanel::renameItem() {
  FileListPane* srcPane = activePane();
  FileListModel* srcModel = srcPane->model();

  QModelIndex currentIndex = srcPane->view()->currentIndex();
  if (!currentIndex.isValid()) {
    return;
  }

  const FileItem* item = srcModel->itemAt(currentIndex);
  if (!item || item->isDotDot()) {
    return;
  }

  QString oldName = item->name();
  QString oldPath = item->absolutePath();

  // Ask for new name
  bool ok;
  QString newName = QInputDialog::getText(
    this,
    tr("Rename"),
    tr("Enter new name:"),
    QLineEdit::Normal,
    oldName,
    &ok
  );

  if (!ok || newName.isEmpty() || newName == oldName) {
    return;
  }

  // Check if new name already exists
  QString parentPath = QFileInfo(oldPath).path();
  QString newPath = parentPath + "/" + newName;

  if (QFileInfo::exists(newPath)) {
    QMessageBox::critical(
      this,
      tr("Error"),
      tr("A file or directory with the name '%1' already exists.").arg(newName)
    );
    return;
  }

  // Rename
  bool success = false;
  if (item->isDir()) {
    QDir dir;
    success = dir.rename(oldPath, newPath);
  } else {
    QFile file;
    success = file.rename(oldPath, newPath);
  }

  if (!success) {
    QMessageBox::critical(
      this,
      tr("Error"),
      tr("Failed to rename '%1' to '%2'.").arg(oldName, newName)
    );
    return;
  }

  // Refresh and move cursor to renamed item
  QString currentPath = srcPane->currentPath();
  srcPane->setPath(currentPath);

  // Find and select the renamed item
  for (int i = 0; i < srcModel->rowCount(); ++i) {
    const FileItem* renamedItem = srcModel->itemAt(i);
    if (renamedItem && renamedItem->name() == newName) {
      srcPane->view()->setCurrentIndex(srcModel->index(i, 0));
      break;
    }
  }

  srcPane->view()->setFocus();
}

} // namespace Farman
